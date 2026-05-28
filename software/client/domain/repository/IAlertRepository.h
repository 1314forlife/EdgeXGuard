// client/domain/repository/IAlertRepository.h
#ifndef IALERTREPOSITORY_H
#define IALERTREPOSITORY_H

#include "../model/alert/AlertInfo.h"
#include <vector>

class IAlertRepository
{
public:
    virtual ~IAlertRepository() = default;

    virtual std::vector<AlertInfo> getRecentAlerts(int limit = 50) = 0;
    virtual bool addAlert(const AlertInfo& alert) = 0;
    virtual bool acknowledgeAlert(int id) = 0;
};

#endif // IALERTREPOSITORY_H