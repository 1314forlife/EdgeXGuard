#ifndef IDEVICEREPOSITORY_H
#define IDEVICEREPOSITORY_H

#include "../model/device/DeviceInfo.h"
#include <vector>
#include <QList>

class IDeviceRepository
{
public:
    virtual ~IDeviceRepository() = default;

    // 设备发现（ONVIF 搜索）
    virtual QList<DeviceInfo> discoverDevices(int timeoutMs = 3000) = 0;

    // 增删改查
    virtual bool addDevice(const DeviceInfo& device) = 0;
    virtual bool removeDevice(const QString& id) = 0;
    virtual bool updateDevice(const DeviceInfo& device) = 0;
    virtual DeviceInfo getDeviceById(const QString& id) = 0;
    virtual std::vector<DeviceInfo> getAllDevices() = 0;

    // 状态更新
    virtual bool updateDeviceState(const QString& id, DeviceState state) = 0;

    // 摄像头专用
    virtual bool updateRtspUrl(const QString& id, const QString& rtspUrl) = 0;
    virtual QString getRtspUrl(const QString& id) = 0;
};

#endif // IDEVICEREPOSITORY_H