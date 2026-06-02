#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QString>
#include <QDateTime>

enum class DeviceType {
    CAMERA,      // 摄像头
    SENSOR,      // 传感器
    ACTUATOR,    // 执行器（风扇、灯光）
    UNKNOWN
};

enum class DeviceState {
    ONLINE,
    OFFLINE,
    UNKNOWN
};

// 执行器状态（开关）
enum class ActuatorState {
    OFF,
    ON,
    UNKNOWN
};

struct DeviceInfo
{
    // 基础信息
    QString id;
    QString name;
    DeviceType type = DeviceType::UNKNOWN;
    DeviceState state = DeviceState::UNKNOWN;
    QString model;
    QString firmware;
    QString manufacturer;
    QString location;

    // 摄像头专用
    QString rtspUrl;
    QString onvifUrl;
    QString username;
    QString password;
    int channel = 1;

    // 执行器专用
    ActuatorState actuatorState = ActuatorState::UNKNOWN;

    // 通用
    bool isOnline = false;
    QDateTime lastSeen;

    // 辅助方法
    QString getTypeString() const {
        switch (type) {
        case DeviceType::CAMERA:   return "摄像头";
        case DeviceType::SENSOR:   return "传感器";
        case DeviceType::ACTUATOR: return "执行器";
        default:                   return "未知";
        }
    }

    QString getStateString() const {
        switch (state) {
        case DeviceState::ONLINE:  return "在线";
        case DeviceState::OFFLINE: return "离线";
        default:                   return "未知";
        }
    }

    QString getActuatorStateString() const {
        switch (actuatorState) {
        case ActuatorState::ON:  return "开启";
        case ActuatorState::OFF: return "关闭";
        default:                 return "未知";
        }
    }
};

#endif // DEVICEINFO_H