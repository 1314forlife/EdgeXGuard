#include "onvif_client.h"
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QUdpSocket>

// ==================== 发现响应结构 ====================
struct ProbeMatch {
    QString serviceAddress;
    QString types;
};

// ==================== OnvifClient 实现 ====================
class OnvifClient::Impl
{
public:
    Impl() : nam(nullptr), connected(false) {}
    ~Impl() { delete nam; }

    // WS-Discovery 设备发现
    QList<OnvifDeviceInfo> discover(int timeoutMs)
    {
        QList<OnvifDeviceInfo> devices;
        QList<ProbeMatch> matches = sendProbe(timeoutMs);

        for (const auto& match : matches) {
            OnvifDeviceInfo info;
            info.serviceAddress = match.serviceAddress;
            devices.append(info);
        }

        return devices;
    }

    // 发送 Probe 请求
    QList<ProbeMatch> sendProbe(int timeoutMs)
    {
        QList<ProbeMatch> matches;
        QUdpSocket socket;

        // Probe 消息模板
        const QString probeMsg =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<Envelope xmlns=\"http://www.w3.org/2003/05/soap-envelope\" "
            "          xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
            "          xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
            "          xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\">"
            "  <Header>"
            "    <wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>"
            "    <wsa:MessageID>urn:uuid:test</wsa:MessageID>"
            "    <wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"
            "  </Header>"
            "  <Body>"
            "    <d:Probe>"
            "      <d:Types>dn:NetworkVideoTransmitter</d:Types>"
            "      <d:Scopes />"
            "    </d:Probe>"
            "  </Body>"
            "</Envelope>";

        // 发送到多播地址
        QByteArray data = probeMsg.toUtf8();
        socket.writeDatagram(data, QHostAddress("239.255.255.250"), 3702);

        // 等待响应
        socket.waitForReadyRead(timeoutMs);
        while (socket.hasPendingDatagrams()) {
            QByteArray buffer;
            buffer.resize(socket.pendingDatagramSize());
            socket.readDatagram(buffer.data(), buffer.size());

            // 解析响应
            QString response(buffer);
            ProbeMatch match;
            // 简单提取服务地址（生产环境需要用 XML 解析）
            int pos = response.indexOf("XAddrs>");
            if (pos != -1) {
                int end = response.indexOf("</", pos);
                match.serviceAddress = response.mid(pos + 7, end - pos - 7);
                matches.append(match);
            }
        }

        return matches;
    }

    QNetworkAccessManager* nam;
    bool connected;
    QString serviceAddress;
    QString username;
    QString password;
};

// ==================== OnvifClient 公共接口 ====================
OnvifClient::OnvifClient(QObject* parent)
    : QObject(parent)
    , m_impl(new Impl())
{
    m_impl->nam = new QNetworkAccessManager(this);
}

OnvifClient::~OnvifClient()
{
    delete m_impl;
}

QList<OnvifDeviceInfo> OnvifClient::discoverDevices(int timeoutMs)
{
    return m_impl->discover(timeoutMs);
}

bool OnvifClient::connectToDevice(const QString& serviceAddress, const QString& username, const QString& password)
{
    m_impl->serviceAddress = serviceAddress;
    m_impl->username = username;
    m_impl->password = password;
    m_impl->connected = true;
    return true;
}

OnvifDeviceInfo OnvifClient::getDeviceInfo()
{
    OnvifDeviceInfo info;
    // TODO: 调用 GetDeviceInformation 接口
    return info;
}

QString OnvifClient::getStreamUri()
{
    // TODO: 调用 GetStreamUri 接口
    return QString();
}

bool OnvifClient::continuousMove(double x, double y, double zoom)
{
    // TODO: 调用 ContinuousMove 接口
    Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(zoom);
    return false;
}

bool OnvifClient::stopMove()
{
    // TODO: 调用 Stop 接口
    return false;
}

bool OnvifClient::gotoPreset(const QString& presetToken)
{
    // TODO: 调用 GotoPreset 接口
    Q_UNUSED(presetToken);
    return false;
}