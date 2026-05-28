#ifndef ALERTVIEWMODEL_H
#define ALERTVIEWMODEL_H

#include <QObject>
#include <QVariantList>
#include "domain/model/alert/AlertInfo.h"

class AlertViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList alerts READ alerts NOTIFY alertsChanged)

public:
    static AlertViewModel& instance();

    QVariantList alerts() const;

public slots:
    void refresh();
    void acknowledgeAlert(int id);

signals:
    void alertsChanged();
    void newAlert(const QString& message, const QString& level);

private:
    AlertViewModel();
    void updateAlerts(const std::vector<AlertInfo>& alertList);

    QVariantList m_alerts;
};

#endif // ALERTVIEWMODEL_H