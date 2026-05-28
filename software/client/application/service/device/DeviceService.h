#ifndef DEVICESERVICE_H
#define DEVICESERVICE_H

#include "domain/model/device/DeviceInfo.h"
#include "domain/repository/IDeviceRepository.h"
#include <QObject>
#include <vector>

class DeviceService : public QObject
{
    Q_OBJECT

public:
    explicit DeviceService(IDeviceRepository* repo, QObject* parent = nullptr);

    std::vector<DeviceInfo> getAllDevices();
    bool controlDevice(const QString& id, bool turnOn);

private:
    IDeviceRepository* m_repo;
};

#endif // DEVICESERVICE_H