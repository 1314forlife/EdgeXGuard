#ifndef _SERVO_CONTROL_H_
#define _SERVO_CONTROL_H_

// 初始化 PWM 硬件/引脚
void servo_init(void);

// 执行动作：open 为真则转动到开锁角度，否则回位
void servo_set_state(int open);

#endif