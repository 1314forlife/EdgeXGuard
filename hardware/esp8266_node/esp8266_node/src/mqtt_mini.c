#include "mqtt_mini.h"
#include <string.h>

// 📦 物理组装 MQTT CONNECT 报文（固定报文头 + 可变报文头 + 载荷）
int mqtt_pack_connect(uint8_t *buf, const char *client_id) {
    uint8_t *ptr = buf;
    int id_len = strlen(client_id);
    
    *ptr++ = 0x10; // MQTT Control Packet Type: CONNECT (0x10)
    ptr++;         // 留出位置填写 Remaining Length（剩余长度）

    // 可变报文头 (Protocol Name & Level)
    *ptr++ = 0x00; *ptr++ = 0x04; // 协议名长度: 4
    *ptr++ = 'M';  *ptr++ = 'Q';  *ptr++ = 'T'; *ptr++ = 'T';
    *ptr++ = 0x04; // Protocol Level: MQTT v3.1.1 (4)
    *ptr++ = 0x02; // Connect Flags: Clean Session Only (0x02)
    *ptr++ = 0x00; *ptr++ = 0x3C; // Keep Alive: 60秒 (0x003C)

    // 载荷 (Payload: Client ID)
    *ptr++ = (id_len >> 8) & 0xFF;
    *ptr++ = id_len & 0xFF;
    memcpy(ptr, client_id, id_len);
    ptr += id_len;

    // 回填真正的物理剩余长度
    buf[1] = (ptr - buf) - 2;
    return (ptr - buf);
}

// 📦 物理组装 MQTT PUBLISH 报文 (QoS 0 无确认模式，速度极快)
int mqtt_pack_publish(uint8_t *buf, const char *topic, const char *payload) {
    uint8_t *ptr = buf;
    int topic_len = strlen(topic);
    int payload_len = strlen(payload);

    *ptr++ = 0x30; // MQTT Control Packet Type: PUBLISH (QoS 0)
    ptr++;         // 留出位置填写 Remaining Length

    // 可变报文头 (Topic Name)
    *ptr++ = (topic_len >> 8) & 0xFF;
    *ptr++ = topic_len & 0xFF;
    memcpy(ptr, topic, topic_len);
    ptr += topic_len;

    // 载荷 (Payload: JSON 字符串)
    memcpy(ptr, payload, payload_len);
    ptr += payload_len;

    // 回填剩余长度
    buf[1] = (ptr - buf) - 2;
    return (ptr - buf);
}