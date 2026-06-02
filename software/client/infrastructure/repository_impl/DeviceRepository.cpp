#include "DeviceRepository.h"
#include <QDebug>

DeviceRepository::DeviceRepository()
{
    // 执行器
    DeviceInfo fan;
    fan.id = "fan_1";
    fan.name = "风扇";
    fan.type = DeviceType::ACTUATOR;
    fan.state = DeviceState::ONLINE;
    fan.actuatorState = ActuatorState::OFF;
    fan.isOnline = true;
    m_devices[fan.id] = fan;

    DeviceInfo light;
    light.id = "light_1";
    light.name = "灯光";
    light.type = DeviceType::ACTUATOR;
    light.state = DeviceState::ONLINE;
    light.actuatorState = ActuatorState::OFF;
    light.isOnline = true;
    m_devices[light.id] = light;

    // 示例摄像头（打桩数据）
    DeviceInfo camera;
    camera.id = "cam_1";
    camera.name = "前门摄像头";
    camera.type = DeviceType::CAMERA;
    camera.state = DeviceState::ONLINE;
    camera.isOnline = true;
    camera.rtspUrl = "rtsp://admin:z13312555@192.168.0.103:554/stream1";
    camera.onvifUrl = "http://192.168.0.103:2020/onvif/device_service";
    camera.model = "TL-IPC45CL-V2";
    camera.location = "前门";
    m_devices[camera.id] = camera;
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
    m_devices[id].isOnline = (state == DeviceState::ONLINE);
    return true;
}

// ========== 新增方法 ==========

QList<DeviceInfo> DeviceRepository::discoverDevices(int timeoutMs)
{
    QList<DeviceInfo> devices;

    // TODO: 实现 ONVIF WS-Discovery
    // 目前返回打桩数据
    DeviceInfo cam;
    cam.id = "cam_discovered_1";
    cam.name = "发现摄像头";
    cam.type = DeviceType::CAMERA;
    cam.state = DeviceState::ONLINE;
    cam.onvifUrl = "http://192.168.0.103:2020/onvif/device_service";
    devices.append(cam);

    return devices;
}

bool DeviceRepository::addDevice(const DeviceInfo& device)
{
    if (m_devices.contains(device.id)) {
        return false;
    }
    m_devices[device.id] = device;
    return true;
}

bool DeviceRepository::removeDevice(const QString& id)
{
    if (!m_devices.contains(id)) {
        return false;
    }
    m_devices.remove(id);
    return true;
}

bool DeviceRepository::updateDevice(const DeviceInfo& device)
{
    if (!m_devices.contains(device.id)) {
        return false;
    }
    m_devices[device.id] = device;
    return true;
}

bool DeviceRepository::updateRtspUrl(const QString& id, const QString& rtspUrl)
{
    if (!m_devices.contains(id)) {
        return false;
    }
    m_devices[id].rtspUrl = rtspUrl;
    return true;
}

QString DeviceRepository::getRtspUrl(const QString& id)
{
    if (m_devices.contains(id)) {
        return m_devices[id].rtspUrl;
    }
    return QString();
}