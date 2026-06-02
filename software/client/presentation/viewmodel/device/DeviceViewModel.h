#ifndef DEVICEVIEWMODEL_H
#define DEVICEVIEWMODEL_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include "domain/model/device/DeviceInfo.h"

class DeviceViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)

public:
    static DeviceViewModel& instance();

    QVariantList devices() const;

    // 设备详情查询
    Q_INVOKABLE QString getDeviceName(const QString& id);
    Q_INVOKABLE QString getDeviceType(const QString& id);
    Q_INVOKABLE bool getDeviceOnline(const QString& id);
    Q_INVOKABLE QString getRtspUrl(const QString& id);
    Q_INVOKABLE QString getOnvifUrl(const QString& id);
    Q_INVOKABLE QString getModel(const QString& id);
    Q_INVOKABLE QString getLocation(const QString& id);
    Q_INVOKABLE bool getActuatorState(const QString& id);

public slots:
    void refresh();
    bool controlDevice(const QString& id, bool turnOn);

signals:
    void devicesChanged();
    void deviceStateChanged(const QString& id, bool isOn);

private:
    DeviceViewModel();
    void updateDevices(const std::vector<DeviceInfo>& deviceList);
    DeviceInfo getDeviceInfoById(const QString& id);

    QVariantList m_devices;
    std::vector<DeviceInfo> m_deviceInfoList;  // 缓存完整设备信息
};

#endif // DEVICEVIEWMODEL_H