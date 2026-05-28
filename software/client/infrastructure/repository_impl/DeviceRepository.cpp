#include "DeviceRepository.h"

DeviceRepository::DeviceRepository()
{
    DeviceInfo fan;
    fan.id = "fan_1";
    fan.name = "风扇";
    fan.type = DeviceType::FAN;
    fan.state = DeviceState::OFF;
    fan.isOnline = true;
    m_devices[fan.id] = fan;

    DeviceInfo light;
    light.id = "light_1";
    light.name = "灯光";
    light.type = DeviceType::LIGHT;
    light.state = DeviceState::OFF;
    light.isOnline = true;
    m_devices[light.id] = light;
}

std::vector<DeviceInfo> DeviceRepository::getAllDevices()
{
    std::vector<DeviceInfo> result;
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        result.push_back(it.value());
    }
    return result;
}

DeviceInfo DeviceRepository::getDeviceById(const QString& id)
{
    if (m_devices.contains(id)) {
        return m_devices[id];
    }
    DeviceInfo empty;
    empty.id = id;
    empty.isOnline = false;
    return empty;
}

bool DeviceRepository::updateDeviceState(const QString& id, DeviceState state)
{
    if (!m_devices.contains(id)) {
        return false;
    }
    m_devices[id].state = state;
    return true;
}