#include "AlertViewModel.h"
#include "application/service/alert/AlertService.h"
#include "infrastructure/repository_impl/AlertRepository.h"
#include <QTimer>

AlertViewModel& AlertViewModel::instance()
{
    static AlertViewModel vm;
    return vm;
}

AlertViewModel::AlertViewModel()
{
    static AlertRepository repo;
    static AlertService service(&repo);

    // 连接 Service 的信号
    QObject::connect(&service, &AlertService::alertAdded,
                     [this](const AlertInfo& alert) {
                         refresh();
                         emit newAlert(alert.message, alert.getLevelString());
                     });

    // 立即刷新一次
    refresh();
}

void AlertViewModel::updateAlerts(const std::vector<AlertInfo>& alertList)
{
    m_alerts.clear();
    for (const auto& alert : alertList) {
        QVariantMap map;
        map["id"] = alert.id;
        map["type"] = alert.getTypeString();
        map["level"] = alert.getLevelString();
        map["levelColor"] = alert.getLevelColor();
        map["message"] = alert.message;
        map["time"] = alert.timestamp.toString("hh:mm:ss");
        map["acknowledged"] = alert.acknowledged;
        m_alerts.append(map);
    }
    emit alertsChanged();
}

QVariantList AlertViewModel::alerts() const
{
    return m_alerts;
}

void AlertViewModel::refresh()
{
    static AlertRepository repo;
    static AlertService service(&repo);
    auto alerts = service.getRecentAlerts(20);
    updateAlerts(alerts);
}

void AlertViewModel::acknowledgeAlert(int id)
{
    static AlertRepository repo;
    static AlertService service(&repo);
    service.acknowledgeAlert(id);
    refresh();
}