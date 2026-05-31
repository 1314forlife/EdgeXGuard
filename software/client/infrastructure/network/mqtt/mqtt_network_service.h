#ifndef MQTT_NETWORK_SERVICE_H
#define MQTT_NETWORK_SERVICE_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <mqtt/async_client.h>  // 直接包含完整头文件

class MqttNetworkService : public QObject
{
    Q_OBJECT

public:
    explicit MqttNetworkService(QObject *parent = nullptr);
    ~MqttNetworkService();

    void connectToGateway(const QString &host, quint16 port);
    void disconnectFromGateway();
    void publishControlCommand(const QString &topic, const QString &command);

signals:
    void gatewayConnected();
    void gatewayDisconnected();
    void alarmDataReceived(const QString &topic, const QByteArray &payload);

private:
    class Impl;
    Impl* m_impl;
};

#endif