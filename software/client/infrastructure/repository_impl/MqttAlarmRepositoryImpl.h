#ifndef MQTT_ALARM_REPOSITORY_IMPL_H
#define MQTT_ALARM_REPOSITORY_IMPL_H

#include "domain/repository/IAlarmRepository.h"

class MqttAlarmRepositoryImpl : public IAlarmRepository {
public:
    MqttAlarmRepositoryImpl() = default;
    ~MqttAlarmRepositoryImpl() override = default;

    bool triggerAlarm(int durationMs) override;
    bool stopAlarm() override;
};

#endif // MQTT_ALARM_REPOSITORY_IMPL_H