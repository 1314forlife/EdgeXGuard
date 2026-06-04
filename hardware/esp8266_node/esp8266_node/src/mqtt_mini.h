#ifndef __MQTT_MINI_H__
#define __MQTT_MINI_H__

#include <stdint.h>

// 📡 根据 MQTT 3.1.1 协议标准手造的极简打包器
// 负责把 Client ID、Topic、Payload 物理揉成标准 TCP 字节流
int mqtt_pack_connect(uint8_t *buf, const char *client_id);
int mqtt_pack_publish(uint8_t *buf, const char *topic, const char *payload);

#endif