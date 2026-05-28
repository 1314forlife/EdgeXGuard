#include "SensorService.h"

SensorService::SensorService(ISensorRepository* repo, QObject* parent)
    : QObject(parent)
    , m_repo(repo)
{
}

SensorData SensorService::getCurrentData()
{
    return m_repo->getCurrentData();
}

void SensorService::refreshData()
{
    SensorData data = m_repo->getCurrentData();
    emit dataUpdated(data);
}