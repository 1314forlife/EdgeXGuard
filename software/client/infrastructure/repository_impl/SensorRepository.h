#ifndef SENSORREPOSITORY_H
#define SENSORREPOSITORY_H

#include "domain/repository/ISensorRepository.h"

class SensorRepository : public ISensorRepository
{
public:
    SensorData getCurrentData() override;
    bool updateSensorData(const SensorData& data) override;
};

#endif // SENSORREPOSITORY_H