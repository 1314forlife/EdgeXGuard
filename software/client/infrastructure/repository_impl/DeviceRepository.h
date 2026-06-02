#ifndef DEVICEREPOSITORY_H
#define DEVICEREPOSITORY_H

#include "domain/repository/IDeviceRepository.h"
#include <QMap>

class DeviceRepository : public IDeviceRepository
{
public:
    DeviceRepository();

    // IDeviceRepository 接口实现
    std::vector<DeviceInfo> getAllDevices() override;
    DeviceInfo getDeviceById(const QString& id) override;
    bool updateDeviceState(const QString& id, DeviceState state) override;

    // 新增方法
    QList<DeviceInfo> discoverDevices(int timeoutMs = 3000) override;
    bool addDevice(const DeviceInfo& device) override;
    bool removeDevice(const QString& id) override;
    bool updateDevice(const DeviceInfo& device) override;
    bool updateRtspUrl(const QString& id, const QString& rtspUrl) override;
    QString getRtspUrl(const QString& id) override;

private:
    QMap<QString, DeviceInfo> m_devices;
};

#endif // DEVICEREPOSITORY_H