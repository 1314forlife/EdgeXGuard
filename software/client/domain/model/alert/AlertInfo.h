#ifndef ALERTINFO_H
#define ALERTINFO_H

#include <QString>
#include <QDateTime>

enum class AlertLevel {
    INFO,
    WARNING,
    CRITICAL
};

enum class AlertType {
    HIGH_TEMPERATURE,
    LOW_TEMPERATURE,
    HIGH_HUMIDITY,
    PEOPLE_DETECTED,
    DEVICE_OFFLINE,
    AI_DETECTION
};

struct AlertInfo
{
    int id = 0;
    AlertType type = AlertType::HIGH_TEMPERATURE;
    AlertLevel level = AlertLevel::INFO;
    QString message;
    QDateTime timestamp;
    bool acknowledged = false;

    QString getTypeString() const;
    QString getLevelString() const;
    QString getLevelColor() const;
};

#endif // ALERTINFO_H