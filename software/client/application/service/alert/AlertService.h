#ifndef ALERTSERVICE_H
#define ALERTSERVICE_H

#include "domain/model/alert/AlertInfo.h"
#include "domain/repository/IAlertRepository.h"
#include <QObject>
#include <vector>

class AlertService : public QObject
{
    Q_OBJECT

public:
    explicit AlertService(IAlertRepository* repo, QObject* parent = nullptr);

    std::vector<AlertInfo> getRecentAlerts(int limit = 20);
    void addAlert(AlertType type, AlertLevel level, const QString& message);
    void acknowledgeAlert(int id);

signals:
    void alertAdded(const AlertInfo& alert);

private:
    IAlertRepository* m_repo;
};

#endif // ALERTSERVICE_H