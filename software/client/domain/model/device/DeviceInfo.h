#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QString>

enum class DeviceType {
    FAN,
    LIGHT,
    UNKNOWN
};

enum class DeviceState {
    OFF,
    ON,
    UNKNOWN
};

struct DeviceInfo
{
    QString id;
    QString name;
    DeviceType type = DeviceType::UNKNOWN;
    DeviceState state = DeviceState::UNKNOWN;
    bool isOnline = false;

    QString getTypeString() const {
        switch (type) {
        case DeviceType::FAN:   return "风扇";
        case DeviceType::LIGHT: return "灯光";
        default:                return "未知";
        }
    }

    QString getStateString() const {
        switch (state) {
        case DeviceState::ON:  return "开启";
        case DeviceState::OFF: return "关闭";
        default:               return "未知";
        }
    }
};

#endif // DEVICEINFO_H