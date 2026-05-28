#ifndef SENSORSERVICE_H
#define SENSORSERVICE_H

#include "domain/model/sensor/SensorData.h"
#include "domain/repository/ISensorRepository.h"
#include <QObject>

class SensorService : public QObject
{
    Q_OBJECT

public:
    explicit SensorService(ISensorRepository* repo, QObject* parent = nullptr);

    SensorData getCurrentData();
    void refreshData();

signals:
    void dataUpdated(const SensorData& data);

private:
    ISensorRepository* m_repo;
};

#endif // SENSORSERVICE_H