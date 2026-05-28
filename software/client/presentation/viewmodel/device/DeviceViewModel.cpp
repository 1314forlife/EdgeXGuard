#include "DeviceViewModel.h"
#include "application/service/device/DeviceService.h"
#include "infrastructure/repository_impl/DeviceRepository.h"

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
    m_devices.clear();
    for (const auto& device : deviceList) {
        QVariantMap map;
        map["id"] = device.id;
        map["name"] = device.name;
        map["type"] = device.getTypeString();
        map["isOn"] = (device.state == DeviceState::ON);
        map["isOnline"] = device.isOnline;
        m_devices.append(map);
    }
    emit devicesChanged();
}

QVariantList DeviceViewModel::devices() const
{
    return m_devices;
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