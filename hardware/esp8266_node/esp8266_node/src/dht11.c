#include "dht11.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h"        
#include "esp_common.h"  

#define DHT11_PIN 4      // GPIO4 对应物理丝印 D2

// 强制编译器不准优化这个延时循环
static void inline volatile delay_us(uint32_t us) {
    os_delay_us(us); 
}

bool dht11_read_raw(float *temperature, int *humidity) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint32_t timeout = 0;

    // 1. 主机发送起始信号：彻底拉低总线 20ms 确保传感器能听到
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4); 
    GPIO_OUTPUT_SET(DHT11_PIN, 0); 
    vTaskDelay(22 / portTICK_RATE_MS); // 拉低 22ms，绝对满足 DHT11 的 >18ms 要求

    // 2. 主机释放总线，转为输入状态，并开启上拉电阻（防止悬空噪声）
    GPIO_OUTPUT_SET(DHT11_PIN, 1);
    delay_us(30);
    GPIO_DIS_OUTPUT(DHT11_PIN); 

    // 3. 进入原子临界区，锁死系统调度，专心数微秒
    portENTER_CRITICAL(); 

    // 等待 DHT11 的低电平响应
    timeout = 0;
    while (GPIO_INPUT_GET(DHT11_PIN) == 1) {
        if (timeout++ > 200) { portEXIT_CRITICAL(); return false; } // 放宽超时
        delay_us(1);
    }

    // 测量 DHT11 发出的 80us 低电平响应
    timeout = 0;
    while (GPIO_INPUT_GET(DHT11_PIN) == 0) {
        if (timeout++ > 200) { portEXIT_CRITICAL(); return false; }
        delay_us(1);
    }

    // 测量 DHT11 发出的 80us 高电平准备信号
    timeout = 0;
    while (GPIO_INPUT_GET(DHT11_PIN) == 1) {
        if (timeout++ > 200) { portEXIT_CRITICAL(); return false; }
        delay_us(1);
    }

    // 4. 开始接收 40 位数据
    for (int i = 0; i < 40; i++) {
        // 每一位的开头是 50us 的低电平标志
        timeout = 0;
        while (GPIO_INPUT_GET(DHT11_PIN) == 0) {
            if (timeout++ > 200) { portEXIT_CRITICAL(); return false; }
            delay_us(1);
        }

        // 核心微秒肉搏：数高电平持续了多少微秒
        uint32_t us_count = 0;
        while (GPIO_INPUT_GET(DHT11_PIN) == 1) {
            us_count++;
            delay_us(1);
            if (us_count > 200) { portEXIT_CRITICAL(); return false; } // 死锁保护
        }

        int byte_idx = i / 8;
        data[byte_idx] <<= 1;
        
        // 旧版 SDK 晶振下，如果高电平循环计数大于 25 (约 40-50us)，判定为二进制 1
        if (us_count > 25) { 
            data[byte_idx] |= 1;
        }
    }

    // 5. 安全退出临界区，恢复全局中断
    portEXIT_CRITICAL(); 

    // 6. 校验和（极其重要：防止读出死数据）
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        // 如果数据全为0，依然视为噪声干扰
        if (data[0] == 0 && data[2] == 0) return false;

        *humidity = data[0];
        *temperature = (float)data[2] + ((float)data[3] / 10.0f);
        return true;
    }

    return false;
}