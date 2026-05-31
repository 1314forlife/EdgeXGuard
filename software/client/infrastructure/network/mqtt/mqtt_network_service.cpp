#include "mqtt_network_service.h"
#include <QDebug>
#include <QMetaObject>
#include <mqtt/async_client.h>

class MqttNetworkService::Impl : public virtual mqtt::callback
{
public:
    Impl(MqttNetworkService* service) : m_service(service), m_client(nullptr) {}

    ~Impl() {
        if (m_client && m_client->is_connected()) {
            m_client->disconnect()->wait();
        }
        delete m_client;
    }

    void connectToGateway(const QString& host, quint16 port) {
        std::string serverURI = QString("tcp://%1:%2").arg(host).arg(port).toStdString();

        if (m_client) {
            delete m_client;
            m_client = nullptr;
        }

        m_client = new mqtt::async_client(serverURI, "qt_client_" + std::to_string(reinterpret_cast<uintptr_t>(this)));

        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);

        try {
            m_client->set_callback(*this);
            m_client->connect(connOpts)->wait();
            qDebug() << "MQTT connected to" << serverURI.c_str();
        } catch (const std::exception& e) {
            qWarning() << "MQTT connection error:" << e.what();
        }
    }

    void disconnectFromGateway() {
        if (m_client && m_client->is_connected()) {
            try {
                m_client->disconnect()->wait();
                qDebug() << "MQTT disconnected";
            } catch (const std::exception& e) {
                qWarning() << "MQTT disconnect error:" << e.what();
            }
        }
    }

    void publishControlCommand(const QString& topic, const QString& command) {
        if (!m_client || !m_client->is_connected()) {
            qWarning() << "MQTT not connected, cannot publish";
            return;
        }

        try {
            auto msg = mqtt::make_message(topic.toStdString(), command.toStdString());
            msg->set_qos(1);
            m_client->publish(msg);
            qDebug() << "Published to" << topic << ":" << command;
        } catch (const std::exception& e) {
            qWarning() << "MQTT publish error:" << e.what();
        }
    }

    void connected(const std::string& cause) override {
        Q_UNUSED(cause);
        QMetaObject::invokeMethod(m_service, "gatewayConnected", Qt::QueuedConnection);
    }

    void connection_lost(const std::string& cause) override {
        Q_UNUSED(cause);
        QMetaObject::invokeMethod(m_service, "gatewayDisconnected", Qt::QueuedConnection);
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        if (msg) {
            QString topic = QString::fromStdString(msg->get_topic());
            QByteArray payload = QByteArray::fromStdString(msg->to_string());
            QMetaObject::invokeMethod(m_service, "alarmDataReceived",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, topic),
                                      Q_ARG(QByteArray, payload));
        }
    }

    void delivery_complete(mqtt::delivery_token_ptr token) override {
        Q_UNUSED(token);
    }

private:
    MqttNetworkService* m_service;
    mqtt::async_client* m_client;
};

// ==================== MqttNetworkService 实现 ====================

MqttNetworkService::MqttNetworkService(QObject* parent)
    : QObject(parent)
    , m_impl(new Impl(this))
{
}

MqttNetworkService::~MqttNetworkService()
{
    delete m_impl;
}

void MqttNetworkService::connectToGateway(const QString& host, quint16 port)
{
    m_impl->connectToGateway(host, port);
}

void MqttNetworkService::disconnectFromGateway()
{
    m_impl->disconnectFromGateway();
}

void MqttNetworkService::publishControlCommand(const QString& topic, const QString& command)
{
    m_impl->publishControlCommand(topic, command);
}