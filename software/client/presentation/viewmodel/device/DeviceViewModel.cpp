#include "DeviceViewModel.h"
#include "application/service/device/DeviceService.h"
#include "infrastructure/repository_impl/DeviceRepository.h"
#include <QDebug>

DeviceViewModel& DeviceViewModel::instance()
{
    static DeviceViewModel vm;
    return vm;
}

DeviceViewModel::DeviceViewModel()
{
    refresh();
}

void DeviceViewModel::updateDevices(const std::vector<DeviceInfo>& deviceList)
{
    m_deviceInfoList = deviceList;  // 缓存完整信息

    m_devices.clear();
    for (const auto& device : deviceList) {
        QVariantMap map;
        map["id"] = device.id;
        map["name"] = device.name;
        map["type"] = device.getTypeString();
        map["isOnline"] = device.isOnline;

        // 执行器专用
        if (device.type == DeviceType::ACTUATOR) {
            map["isOn"] = (device.actuatorState == ActuatorState::ON);
        } else {
            map["isOn"] = false;
        }

        // 摄像头专用字段
        if (device.type == DeviceType::CAMERA) {
            map["rtspUrl"] = device.rtspUrl;
            map["onvifUrl"] = device.onvifUrl;
            map["model"] = device.model;
            map["location"] = device.location;
        }

        m_devices.append(map);
    }
    emit devicesChanged();
}

QVariantList DeviceViewModel::devices() const
{
    return m_devices;
}

DeviceInfo DeviceViewModel::getDeviceInfoById(const QString& id)
{
    for (const auto& device : m_deviceInfoList) {
        if (device.id == id) {
            return device;
        }
    }
    return DeviceInfo();
}

QString DeviceViewModel::getDeviceName(const QString& id)
{
    return getDeviceInfoById(id).name;
}

QString DeviceViewModel::getDeviceType(const QString& id)
{
    return getDeviceInfoById(id).getTypeString();
}

bool DeviceViewModel::getDeviceOnline(const QString& id)
{
    return getDeviceInfoById(id).isOnline;
}

QString DeviceViewModel::getRtspUrl(const QString& id)
{
    return getDeviceInfoById(id).rtspUrl;
}

QString DeviceViewModel::getOnvifUrl(const QString& id)
{
    return getDeviceInfoById(id).onvifUrl;
}

QString DeviceViewModel::getModel(const QString& id)
{
    return getDeviceInfoById(id).model;
}

QString DeviceViewModel::getLocation(const QString& id)
{
    return getDeviceInfoById(id).location;
}

bool DeviceViewModel::getActuatorState(const QString& id)
{
    auto info = getDeviceInfoById(id);
    return (info.actuatorState == ActuatorState::ON);
}

void DeviceViewModel::refresh()
{
    static DeviceRepository repo;
    static DeviceService service(&repo);
    auto devices = service.getAllDevices();
    updateDevices(devices);
}

bool DeviceViewModel::controlDevice(const QString& id, bool turnOn)
{
    static DeviceRepository repo;
    static DeviceService service(&repo);
    bool result = service.controlDevice(id, turnOn);
    if (result) {
        refresh();
        emit deviceStateChanged(id, turnOn);
    }
    return result;
}