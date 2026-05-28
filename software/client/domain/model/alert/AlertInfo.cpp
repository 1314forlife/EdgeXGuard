#include "AlertInfo.h"

QString AlertInfo::getTypeString() const
{
    switch (type) {
    case AlertType::HIGH_TEMPERATURE: return "温度过高";
    case AlertType::LOW_TEMPERATURE:  return "温度过低";
    case AlertType::HIGH_HUMIDITY:    return "湿度过高";
    case AlertType::PEOPLE_DETECTED:  return "检测到人";
    case AlertType::DEVICE_OFFLINE:   return "设备离线";
    case AlertType::AI_DETECTION:     return "AI检测";
    default: return "未知";
    }
}

QString AlertInfo::getLevelString() const
{
    switch (level) {
    case AlertLevel::INFO:     return "提示";
    case AlertLevel::WARNING:  return "警告";
    case AlertLevel::CRITICAL: return "严重";
    default: return "未知";
    }
}

QString AlertInfo::getLevelColor() const
{
    switch (level) {
    case AlertLevel::INFO:     return "#4CAF50";
    case AlertLevel::WARNING:  return "#FF9800";
    case AlertLevel::CRITICAL: return "#F44336";
    default: return "#9E9E9E";
    }
}