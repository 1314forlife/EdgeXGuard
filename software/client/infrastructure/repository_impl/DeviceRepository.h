#ifndef DEVICEREPOSITORY_H
#define DEVICEREPOSITORY_H

#include "domain/repository/IDeviceRepository.h"
#include <QMap>

class DeviceRepository : public IDeviceRepository
{
public:
    DeviceRepository();

    std::vector<DeviceInfo> getAllDevices() override;
    DeviceInfo getDeviceById(const QString& id) override;
    bool updateDeviceState(const QString& id, DeviceState state) override;

private:
    QMap<QString, DeviceInfo> m_devices;
};

#endif // DEVICEREPOSITORY_H