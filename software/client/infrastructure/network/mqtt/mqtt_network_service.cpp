#include "mqtt_network_service.h"
#include <QDebug>
#include <QMetaObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
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

        // 随机后缀防止 Client ID 冲突
        std::string uniqueId = "qt_client_desktop_" + std::to_string(QDateTime::currentMSecsSinceEpoch());
        m_client = new mqtt::async_client(serverURI, uniqueId);

        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(30);
        connOpts.set_clean_session(true);

        // 🟢 换上你尊贵的管理员通行证，直接推开 EMQX 大门！
        connOpts.set_user_name("admin");
        connOpts.set_password("Zz13312555281");

        try {
            m_client->set_callback(*this);
            m_client->connect(connOpts)->wait();
            qDebug() << "[MQTT Core] 🎉🤝 成功推开大门！连通 Rock 5T 服务器！";
        } catch (const std::exception& e) {
            qWarning() << "[MQTT Core] ❌ 门禁拒绝，原因:" << e.what();
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
        if (m_client) {
            m_client->subscribe("EdgeXGuard/esp8266/data", 1);
            qDebug() << "[Qt MQTT] 成功订阅单片机数据通道: EdgeXGuard/esp8266/data";
        }
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

            if (topic == "EdgeXGuard/esp8266/data") {
                QJsonDocument doc = QJsonDocument::fromJson(payload);
                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject obj = doc.object();

                    // 1. 提取原始高频多噪数值
                    double rawTemp = obj["temperature"].toDouble();
                    int rawHumi = obj["humidity"].toInt();

                    // ─── 🧮 一阶低通滤波器：全部对齐 m_ 级别的类成员变量 ───
                    if (m_filteredTemp == -999.0) {
                        m_filteredTemp = rawTemp;
                        m_filteredHumi = static_cast<double>(rawHumi);
                    } else {
                        // 🟢 滤波系数（0.1~0.3）。取 0.15 兼顾顺滑和反应速度
                        double alpha = 0.15;

                        m_filteredTemp = (alpha * rawTemp) + ((1.0 - alpha) * m_filteredTemp);
                        m_filteredHumi = (alpha * static_cast<double>(rawHumi)) + ((1.0 - alpha) * m_filteredHumi);
                    }
                    // ─── 🛡️ 滤波结束 ───

                    // 打印去噪结果到控制台
                    qDebug() << "[Network Core] 🟢 成功解析物理报文 -> 原始温度:" << rawTemp << "-> 稳定过滤值:" << m_filteredTemp;

                    // 2. 🟢 将彻底焊死、永不重置的成员变量发射给业务层
                    QMetaObject::invokeMethod(m_service, "sensorDataUpdated",
                                              Qt::QueuedConnection,
                                              Q_ARG(double, m_filteredTemp),
                                              Q_ARG(int, static_cast<int>(m_filteredHumi + 0.5))); // 四舍五入转 int
                }
            } else {
                QMetaObject::invokeMethod(m_service, "alarmDataReceived",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, topic),
                                          Q_ARG(QByteArray, payload));
            }
        }
    }

    void delivery_complete(mqtt::delivery_token_ptr token) override {
        Q_UNUSED(token);
    }

private:
    MqttNetworkService* m_service;
    mqtt::async_client* m_client;

    double m_filteredTemp = -999.0;
    double m_filteredHumi = -999.0;
};

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