#include "AlertService.h"

AlertService::AlertService(IAlertRepository* repo, QObject* parent)
    : QObject(parent)
    , m_repo(repo)
{
}

std::vector<AlertInfo> AlertService::getRecentAlerts(int limit)
{
    return m_repo->getRecentAlerts(limit);
}

void AlertService::addAlert(AlertType type, AlertLevel level, const QString& message)
{
    AlertInfo alert;
    alert.type = type;
    alert.level = level;
    alert.message = message;
    alert.acknowledged = false;

    if (m_repo->addAlert(alert)) {
        std::vector<AlertInfo> alerts = m_repo->getRecentAlerts(1);
        if (!alerts.empty()) {
            emit alertAdded(alerts[0]);
        }
    }
}

void AlertService::acknowledgeAlert(int id)
{
    m_repo->acknowledgeAlert(id);
}