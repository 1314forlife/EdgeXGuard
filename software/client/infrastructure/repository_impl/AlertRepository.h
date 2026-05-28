#ifndef ALERTREPOSITORY_H
#define ALERTREPOSITORY_H

#include "domain/repository/IAlertRepository.h"
#include <QList>

class AlertRepository : public IAlertRepository
{
public:
    std::vector<AlertInfo> getRecentAlerts(int limit) override;
    bool addAlert(const AlertInfo& alert) override;
    bool acknowledgeAlert(int id) override;

private:
    QList<AlertInfo> m_alerts;
    int m_nextId = 1;
};

#endif // ALERTREPOSITORY_H