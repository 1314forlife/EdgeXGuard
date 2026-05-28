#ifndef SENSORVIEWMODEL_H
#define SENSORVIEWMODEL_H

#include <QObject>
#include "domain/model/sensor/SensorData.h"

class SensorViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString temperature READ temperature NOTIFY dataChanged)
    Q_PROPERTY(QString humidity READ humidity NOTIFY dataChanged)
    Q_PROPERTY(QString pirStatus READ pirStatus NOTIFY dataChanged)
    Q_PROPERTY(QString updateTime READ updateTime NOTIFY dataChanged)

public:
    static SensorViewModel& instance();

    QString temperature() const;
    QString humidity() const;
    QString pirStatus() const;
    QString updateTime() const;

public slots:
    void refresh();

signals:
    void dataChanged();

private:
    SensorViewModel();
    void updateData(const SensorData& data);

    SensorData m_data;
};

#endif // SENSORVIEWMODEL_H