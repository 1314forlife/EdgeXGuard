#include "MqttAlarmRepositoryImpl.h"
#include <QDebug>

bool MqttAlarmRepositoryImpl::triggerAlarm(int durationMs) {
    qDebug() << "[MqttAlarmRepo] 【打桩】通过 MQTT 发布警报指令给 ESP8266，持续时长:" << durationMs << "ms";
    return true;
}

bool MqttAlarmRepositoryImpl::stopAlarm() {
    qDebug() << "[MqttAlarmRepo] 【打桩】通过 MQTT 发布停止警报指令";
    return true;
}