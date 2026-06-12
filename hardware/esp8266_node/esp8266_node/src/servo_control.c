#include "servo_control.h"
#include "esp_common.h" // 替换原有的 ets_sys.h，它包含了所有底层的定义
#include "gpio.h"

// 舵机信号线接 D6 (GPIO12)
#define SERVO_GPIO 12

void servo_init(void) {
    // 设置 GPIO12 为输出模式
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
    gpio_output_set(0, 0, BIT12, 0);
    printf("[Servo] 驱动初始化完成，D6口已就绪。\n");
}

// 内部函数：发送脉冲
static void send_pulse(int pulse_us) {
    gpio_output_set(BIT12, 0, BIT12, 0); // 拉高
    ets_delay_us(pulse_us);              // 改为 ets_delay_us，这是 SDK 内部的标准函数
    gpio_output_set(0, BIT12, BIT12, 0); // 拉低
}

void servo_set_state(int open) {
    int pulse = open ? 2500 : 500;
    
    // 循环发送信号
    for(int i = 0; i < 50; i++) {
        send_pulse(pulse);
        ets_delay_us(20000 - pulse);     // 同样使用 ets_delay_us
    }
    printf("[Servo] 动作执行: %s\n", open ? "开门" : "关门");
}