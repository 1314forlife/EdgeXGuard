// client/domain/repository/ISensorRepository.h
#ifndef ISENSORREPOSITORY_H
#define ISENSORREPOSITORY_H

#include "../model/sensor/SensorData.h"

class ISensorRepository
{
public:
    virtual ~ISensorRepository() = default;

    virtual SensorData getCurrentData() = 0;
    virtual bool updateSensorData(const SensorData& data) = 0;
};

#endif // ISENSORREPOSITORY_H