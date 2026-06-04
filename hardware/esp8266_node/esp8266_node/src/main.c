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

// 📡 物理网络坐标参数
#define WIFI_SSID       "TP-LINK_3802"
#define WIFI_PASS       "73156281"
#define ROCK5T_IP       "192.168.0.102" // 你的 Rock 5T 真实 IP
#define MQTT_PORT       1883
#define MQTT_TOPIC      "EdgeXGuard/esp8266/data"

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

// ─── 📡 Task 1：传感器采集任务 ───
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

// ─── 🚀 Task 2：MQTT 发送任务 ───
void v_mqtt_broker_task(void *pvParameters) {
    sensor_data_t received_data;
    char payload[128];
    uint8_t tx_buf[256];
    int sock_fd = -1;

    printf("[MQTT Task] 启动，死等 Wi-Fi 物理通电...\n");
    while (!g_wifi_connected) { vTaskDelay(100 / portTICK_RATE_MS); }

    struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(MQTT_PORT);
    remote_addr.sin_addr.s_addr = inet_addr(ROCK5T_IP);

    for (;;) {
        if (sock_fd < 0) {
            printf("[MQTT] ⚡ 正在尝试物理连接 Rock 5T 远端中转站 [%s:%d]...\n", ROCK5T_IP, MQTT_PORT);
            sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (sock_fd >= 0) {
                if (connect(sock_fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) == 0) {
                    printf("[MQTT] 🤝 TCP 握手成功！开始发射 MQTT CONNECT 握手包...\n");
                    int len = mqtt_pack_connect(tx_buf, "ESP8266_EdgeNode");
                    write(sock_fd, tx_buf, len);
                } else {
                    printf("[MQTT] ❌ 连接中转站失败，5秒后重试...\n");
                    close(sock_fd); sock_fd = -1;
                    vTaskDelay(5000 / portTICK_RATE_MS); 
                    continue;
                }
            }
        }

        // 🟢 修正：换成每 1000 毫秒轮询捞取一次，不进行死等，确保长连接心跳存活，防止断线
        if (xQueueReceive(g_sensor_queue, &received_data, 1000 / portTICK_RATE_MS) == pdPASS) {
            if (sock_fd >= 0 && g_wifi_connected) {
                // 将真实的浮点数格式化为标准 JSON 字符串
                snprintf(payload, sizeof(payload), 
                         "{\"device_id\":\"esp8266_01\",\"temperature\":%.1f,\"humidity\":%d}", 
                         received_data.temperature, received_data.humidity);
                
                // 打包成标准 MQTT PUBLISH 字节流并物理发射出去
                int len = mqtt_pack_publish(tx_buf, MQTT_TOPIC, payload);
                if (write(sock_fd, tx_buf, len) < 0) {
                    printf("[MQTT] ❌ 物理发射失败！网络链路中断，强行关断 Socket 触发重连\n");
                    close(sock_fd); sock_fd = -1;
                } else {
                    printf("[MQTT 🚀] 成功物理横渡！JSON 安全送达 Rock 5T -> %s\n", payload);
                }
            }
        }
    }
}

// ─── 🛠️ 全局唯一主入口 ───
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
        printf("[EdgeX_ESP8266] 全部基建与自研 MQTT 中中间件合体成功！\n");
    }
}