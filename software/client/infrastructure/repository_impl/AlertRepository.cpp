#include "AlertRepository.h"

std::vector<AlertInfo> AlertRepository::getRecentAlerts(int limit)
{
    std::vector<AlertInfo> result;
    int count = 0;
    for (int i = m_alerts.size() - 1; i >= 0 && count < limit; --i, ++count) {
        result.push_back(m_alerts[i]);
    }
    return result;
}

bool AlertRepository::addAlert(const AlertInfo& alert)
{
    AlertInfo newAlert = alert;
    newAlert.id = m_nextId++;
    newAlert.timestamp = QDateTime::currentDateTime();
    m_alerts.append(newAlert);
    return true;
}

bool AlertRepository::acknowledgeAlert(int id)
{
    for (int i = 0; i < m_alerts.size(); ++i) {
        if (m_alerts[i].id == id) {
            m_alerts[i].acknowledged = true;
            return true;
        }
    }
    return false;
}