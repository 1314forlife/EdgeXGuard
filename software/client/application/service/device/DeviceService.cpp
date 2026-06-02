#include "DeviceService.h"

DeviceService::DeviceService(IDeviceRepository* repo, QObject* parent)
    : QObject(parent)
    , m_repo(repo)
{
}

std::vector<DeviceInfo> DeviceService::getAllDevices()
{
    return m_repo->getAllDevices();
}

bool DeviceService::controlDevice(const QString& id, bool turnOn)
{
    // 获取当前设备
    auto device = m_repo->getDeviceById(id);
    if (device.id.isEmpty()) {
        return false;
    }

    // 检查是否是执行器类型
    if (device.type != DeviceType::ACTUATOR) {
        return false;
    }

    // 更新执行器状态
    device.actuatorState = turnOn ? ActuatorState::ON : ActuatorState::OFF;

    // 保存更新
    return m_repo->updateDevice(device);
}