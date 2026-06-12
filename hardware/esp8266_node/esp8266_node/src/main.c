#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// FreeRTOS 绝对核心基建
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// 系统原生网络驱动（古董 BSD Socket 库）
#include "lwip/sockets.h"
#include "esp_wifi.h"
#include "esp_sta.h"

// 引入自研 MQTT 打包器与 DHT11 驱动
#include "mqtt_mini.h"
#include "dht11.h"
#include "servo_control.h" 

// 📡 物理网络坐标参数
#define WIFI_SSID       "TP-LINK_3802"
#define WIFI_PASS       "73156281"
#define ROCK5T_IP       "192.168.0.102" // 你的 Rock 5T 真实 IP
#define MQTT_PORT       1883
#define MQTT_TOPIC      "EdgeXGuard/esp8266/data"
#define MQTT_SUB_TOPIC  "EdgeXGuard/esp8266/cmd"  // 🚀 新增：明确指定订阅的指令 Topic

typedef struct {
    float temperature;
    int humidity;
} sensor_data_t;

static xQueueHandle g_sensor_queue = NULL;
static int g_wifi_connected = 0;

// 固定安全扇区打桩
unsigned int user_rf_cal_sector_set(void) { return 1019; }

// Wi-Fi 事件状态机
void wifi_event_handler(System_Event_t *event) {
    switch (event->event_id) {
        case EVENT_STAMODE_CONNECTED:
            printf("[WiFi] 🟢 物理层已接通路由器！正在等待分配 IP 地址...\n");
            break;
        case EVENT_STAMODE_GOT_IP:
            printf("[WiFi] 🎉 连网成功！拿到局域网 IP 地址！\n");
            g_wifi_connected = 1; 
            break;
        case EVENT_STAMODE_DISCONNECTED:
            printf("[WiFi] ❌ 链路断开！启动底层物理重连...\n");
            g_wifi_connected = 0;
            wifi_station_connect(); 
            break;
        default: break;
    }
}

// ─── 📡 Task 1：传感器采集任务 (完全保持你原版的稳定逻辑，坚决不动) ───
void v_sensor_collect_task(void *pvParameters) {
    sensor_data_t data;
    float local_temp = 0.0f;
    int local_humi = 0;

    printf("[Sensor Task] 采集任务启动...\n");

    for (;;) {
        if (dht11_read_raw(&local_temp, &local_humi)) {
            data.temperature = local_temp;
            data.humidity = local_humi;

            int temp_x10 = (int)(data.temperature * 10);
            int temp_integral = temp_x10 / 10;
            int temp_fraction = temp_x10 % 10;

            printf("[LOCAL_OK] 🟢 本地采集 -> 温度: %d.%d °C | 湿度: %d %%\n", 
                   temp_integral, temp_fraction, data.humidity);

            // 将真实的温湿度数据快递送入队列
            xQueueSend(g_sensor_queue, &data, 0);
        } else {
            printf("[LOCAL_ERR] ❌ 采集失败：时序未对齐。\n");
        }

        vTaskDelay(2000 / portTICK_RATE_MS); 
    }
}

// ─── 🚀 Task 2：MQTT 发送/接收任务 (仅补充订阅与人脸成功打印逻辑) ───
void v_mqtt_broker_task(void *pvParameters) {
    sensor_data_t received_data;
    char payload[128];
    uint8_t tx_buf[256];
    int sock_fd = -1;

    printf("[MQTT Task] 启动，等待 Wi-Fi 物理连接...\n");
    while (!g_wifi_connected) { vTaskDelay(100 / portTICK_RATE_MS); }

    struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(MQTT_PORT);
    remote_addr.sin_addr.s_addr = inet_addr(ROCK5T_IP);

    for (;;) {
        if (sock_fd < 0) {
            printf("[MQTT] ⚡ 正在尝试连接 Rock 5T [%s:%d]...\n", ROCK5T_IP, MQTT_PORT);
            sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (sock_fd >= 0) {
                if (connect(sock_fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) == 0) {
                    printf("[MQTT] 🤝 TCP 握手成功！\n");
                    
                    // 1. 发送连接包
                    int len = mqtt_pack_connect(tx_buf, "ESP8266_EdgeNode");
                    write(sock_fd, tx_buf, len);
                    
                    // 🚀 2. 【核心新增】在这里补充订阅包，告诉 Broker 我们要监听 cmd 主题
                    vTaskDelay(100 / portTICK_RATE_MS); // 稍微延时让连接更稳
                    int sub_len = mqtt_pack_subscribe(tx_buf, MQTT_SUB_TOPIC);
                    write(sock_fd, tx_buf, sub_len);
                    printf("[MQTT] 📡 订阅指令主题成功 -> %s\n", MQTT_SUB_TOPIC);
                } else {
                    close(sock_fd); sock_fd = -1;
                    vTaskDelay(5000 / portTICK_RATE_MS);
                    continue;
                }
            }
        }

        // 1. 发送逻辑：检测是否有传感器数据入队 (保持你原版的逻辑)
        if (xQueueReceive(g_sensor_queue, &received_data, 10 / portTICK_RATE_MS) == pdPASS) {
            if (sock_fd >= 0 && g_wifi_connected) {
                snprintf(payload, sizeof(payload), 
                         "{\"device_id\":\"esp8266_01\",\"temperature\":%.1f,\"humidity\":%d}", 
                         received_data.temperature, received_data.humidity);
                int len = mqtt_pack_publish(tx_buf, MQTT_TOPIC, payload);
                if (write(sock_fd, tx_buf, len) < 0) {
                    printf("[MQTT] ❌ 发送失败，重置 Socket\n");
                    close(sock_fd); sock_fd = -1;
                }
            }
        }

        // 2. 🚀 独立监听逻辑 (非阻塞模式，仅做人脸识别成功后的打印逻辑)
            if (sock_fd >= 0) {
            fcntl(sock_fd, F_SETFL, O_NONBLOCK); // 确保非阻塞
            uint8_t rx_buf[64];
            int n = read(sock_fd, rx_buf, sizeof(rx_buf) - 1);
            if (n > 0) {
                // 🔍 精准调试打印，留着心里踏实
                printf("[DEBUG_RAW] 收到网络数据长度: %d 字节\n", n);
                
                // 🛠️ 核心修改：利用循环在原始字节数组里直接肉眼式搜索 "open"
                int found_open = 0;
                for (int i = 0; i <= n - 4; i++) {
                    if (rx_buf[i]   == 0x6F &&   // 'o'
                        rx_buf[i+1] == 0x70 &&   // 'p'
                        rx_buf[i+2] == 0x65 &&   // 'e'
                        rx_buf[i+3] == 0x6E)     // 'n'
                    {
                        found_open = 1;
                        break;
                    }
                }

                // 🚀 触发人脸识别成功逻辑
                if (found_open) {
                    printf("\n===============================================\n");
                    printf("[EdgeXGuard] 🎉 人脸识别成功！触发开门逻辑！\n");
                    printf("===============================================\n\n");
                }
            }
        }

        // 任务喘息，防止 CPU 过载
        vTaskDelay(50 / portTICK_RATE_MS); 
    }
}

// ─── 🛠️ 全局唯一主入口 (保持原版逻辑，坚坚决不动) ───
void user_init(void) {
    printf("\n============== EdgeXGuard 终极网络大炮完全体启动 ==============\n");

    struct station_config wifi_config;
    memset(&wifi_config, 0, sizeof(struct station_config));
    strcpy((char *)wifi_config.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.password, WIFI_PASS);

    // 开启 Wi-Fi 基础设施
    wifi_set_opmode(STATION_MODE);
    wifi_station_set_config(&wifi_config);
    wifi_set_event_handler_cb(wifi_event_handler);

    // 创建消息队列
    g_sensor_queue = xQueueCreate(10, sizeof(sensor_data_t));

    if (g_sensor_queue != NULL) {
        // 启动两个核心并发任务
        xTaskCreate(v_sensor_collect_task, (const signed char *)"SensorTask", 256, NULL, 5, NULL);
        xTaskCreate(v_mqtt_broker_task,     (const signed char *)"MqttTask",   512, NULL, 3, NULL);
        
        // 物理连接路由器
        wifi_station_connect();
        servo_init();
        printf("[EdgeX_ESP8266] 全部基建与自研 MQTT 中中间件合体成功！\n");
    }
}