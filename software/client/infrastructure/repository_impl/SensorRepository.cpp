#include "SensorRepository.h"
#include <QDateTime>
#include <cstdlib>
#include <ctime>

SensorData SensorRepository::getCurrentData()
{
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        seeded = true;
    }

    SensorData data;
    data.temperature = 20.0f + (std::rand() % 150) / 10.0f;
    data.humidity = 40.0f + (std::rand() % 400) / 10.0f;
    data.pirDetected = (std::rand() % 10) == 0;
    data.timestamp = QDateTime::currentDateTime();

    return data;
}

bool SensorRepository::updateSensorData(const SensorData& data)
{
    Q_UNUSED(data);
    return true;
}