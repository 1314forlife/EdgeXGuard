// src/dht11.h
#ifndef __DHT11_H__
#define __DHT11_H__

#include <stdbool.h>

/**
 * @brief 从物理引脚抓取单总线40位原始温湿度数据
 * @param temperature 接收解析后温度值的浮点数指针
 * @param humidity 接收解析后湿度值的整数指针
 * @return true 采集成功且校验和通过 | false 采集失败
 */
bool dht11_read_raw(float *temperature, int *humidity);

#endif // __DHT11_H__