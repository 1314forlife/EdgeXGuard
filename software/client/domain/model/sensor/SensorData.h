#ifndef SENSORDATA_H
#define SENSORDATA_H

#include <QString>
#include <QDateTime>

struct SensorData
{
    float temperature = 0.0f;
    float humidity = 0.0f;
    bool pirDetected = false;
    QDateTime timestamp;

    bool isHighTemperature(float threshold = 30.0f) const {
        return temperature > threshold;
    }

    bool isLowTemperature(float threshold = 15.0f) const {
        return temperature < threshold;
    }

    bool isHighHumidity(float threshold = 75.0f) const {
        return humidity > threshold;
    }

    bool isPeopleDetected() const {
        return pirDetected;
    }
};

#endif // SENSORDATA_H