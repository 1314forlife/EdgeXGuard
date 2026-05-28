// client/domain/repository/IDeviceRepository.h
#ifndef IDEVICEREPOSITORY_H
#define IDEVICEREPOSITORY_H

#include "../model/device/DeviceInfo.h"
#include <vector>

class IDeviceRepository
{
public:
    virtual ~IDeviceRepository() = default;

    virtual std::vector<DeviceInfo> getAllDevices() = 0;
    virtual DeviceInfo getDeviceById(const QString& id) = 0;
    virtual bool updateDeviceState(const QString& id, DeviceState state) = 0;
};

#endif // IDEVICEREPOSITORY_H