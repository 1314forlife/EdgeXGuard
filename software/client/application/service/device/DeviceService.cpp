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
    DeviceState newState = turnOn ? DeviceState::ON : DeviceState::OFF;
    return m_repo->updateDeviceState(id, newState);
}