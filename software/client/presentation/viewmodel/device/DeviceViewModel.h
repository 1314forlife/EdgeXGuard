#ifndef DEVICEVIEWMODEL_H
#define DEVICEVIEWMODEL_H

#include <QObject>
#include <QVariantList>
#include "domain/model/device/DeviceInfo.h"

class DeviceViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)

public:
    static DeviceViewModel& instance();

    QVariantList devices() const;

public slots:
    void refresh();
    bool controlDevice(const QString& id, bool turnOn);

signals:
    void devicesChanged();
    void deviceStateChanged(const QString& id, bool isOn);

private:
    DeviceViewModel();
    void updateDevices(const std::vector<DeviceInfo>& deviceList);

    QVariantList m_devices;
};

#endif // DEVICEVIEWMODEL_H