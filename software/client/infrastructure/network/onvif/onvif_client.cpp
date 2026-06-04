#include "onvif_client.h"
#include <QUdpSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QDomDocument>
#include <QDebug>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>
#include <QEventLoop>
#include <QtGlobal>

class OnvifClient::Impl {
public:
    QUdpSocket* udpSocket;
    QNetworkAccessManager* networkManager;
    QString username;
    QString password;

    Impl(QObject* parent)
        : udpSocket(new QUdpSocket(parent))
        , networkManager(new QNetworkAccessManager(parent)) {}
};

OnvifClient::OnvifClient(QObject *parent)
    : QObject(parent)
    , m_impl(new Impl(this))
{
    m_impl->udpSocket->bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress);

    connect(m_impl->udpSocket, &QUdpSocket::readyRead, this, [this]() {
        while (m_impl->udpSocket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(m_impl->udpSocket->pendingDatagramSize());
            QHostAddress senderIp;
            m_impl->udpSocket->readDatagram(datagram.data(), datagram.size(), &senderIp);

            QString xmlStr = QString::fromUtf8(datagram);

            // 提取服务地址
            QRegularExpression re("http://[^\\s<]+/device_service");
            auto match = re.match(xmlStr);
            if (match.hasMatch()) {
                OnvifDeviceInfo dev;
                dev.ipAddress = senderIp.toString().remove("::ffff:");
                dev.serviceAddress = match.captured();
                emit deviceDiscovered(dev);
            }
        }
    });
}

OnvifClient::~OnvifClient()
{
    delete m_impl;
}

void OnvifClient::setCredentials(const QString& username, const QString& password)
{
    m_impl->username = username;
    m_impl->password = password;
}

void OnvifClient::discoverDevices()
{
    QByteArray probe =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<Envelope xmlns=\"http://www.w3.org/2003/05/soap-envelope\" "
        "xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\">"
        "<Body><Probe><Types>dn:NetworkVideoTransmitter</Types></Probe></Body>"
        "</Envelope>";

    m_impl->udpSocket->writeDatagram(probe, QHostAddress("239.255.255.250"), 3702);
    qDebug() << "[ONVIF] 发送 Probe 发现请求";
}

void OnvifClient::requestDeviceInfo(const QString& serviceAddress)
{
    QUrl url(serviceAddress);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/soap+xml; charset=utf-8");

    // 使用 WS-UsernameToken 认证（不需要 Basic Auth）
    // 注意：不要添加 Authorization: Basic 头，因为认证信息已经在 SOAP Header 中

    // 使用正确的格式，包含 Nonce + Created + PasswordDigest
    QByteArray msg =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
        "xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" "
        "xmlns:tt=\"http://www.onvif.org/ver10/schema\">" +
        getWsSecurityHeader().toUtf8() +
        "<soap:Body>"
        "<tds:GetDeviceInformation/>"
        "</soap:Body>"
        "</soap:Envelope>";

    qDebug() << "[ONVIF] 请求体:" << msg;

    QNetworkReply* reply = m_impl->networkManager->post(request, msg);
    connect(reply, &QNetworkReply::finished, this, [this, reply, serviceAddress]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString res = QString::fromUtf8(reply->readAll());
            qDebug() << "[ONVIF] 响应:" << res;

            OnvifDeviceInfo info;
            info.serviceAddress = serviceAddress;

            QRegularExpression reManu("<tds:Manufacturer>([^<]+)</tds:Manufacturer>");
            auto match = reManu.match(res);
            if (match.hasMatch()) info.manufacturer = match.captured(1);

            QRegularExpression reModel("<tds:Model>([^<]+)</tds:Model>");
            match = reModel.match(res);
            if (match.hasMatch()) info.hardware = match.captured(1);

            QRegularExpression reFw("<tds:FirmwareVersion>([^<]+)</tds:FirmwareVersion>");
            match = reFw.match(res);
            if (match.hasMatch()) info.firmware = match.captured(1);

            QRegularExpression reSn("<tds:SerialNumber>([^<]+)</tds:SerialNumber>");
            match = reSn.match(res);
            if (match.hasMatch()) info.serialNumber = match.captured(1);

            emit deviceInfoReceived(serviceAddress, info);
        } else {
            qDebug() << "[ONVIF] 请求失败:" << reply->errorString();
            qDebug() << "HTTP 状态码:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        }
        reply->deleteLater();
    });
}

void OnvifClient::requestStreamUri(const QString& serviceAddress)
{
    QString mediaUrl = serviceAddress;
    mediaUrl.replace("device_service", "media_service");

    QUrl url(mediaUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/soap+xml; charset=utf-8");

    // 先获取 Profiles
    QByteArray getProfilesMsg =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
        "xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\">" +
        getWsSecurityHeader().toUtf8() +
        "<soap:Body><trt:GetProfiles/></soap:Body>"
        "</soap:Envelope>";

    QNetworkReply* reply = m_impl->networkManager->post(request, getProfilesMsg);
    connect(reply, &QNetworkReply::finished, this, [this, reply, serviceAddress]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString res = QString::fromUtf8(reply->readAll());
            qDebug() << "[ONVIF] GetProfiles 响应:" << res;

            // 提取 ProfileToken
            QRegularExpression re("token=\"([^\"]+)\"");
            auto match = re.match(res);
            QString profileToken = match.hasMatch() ? match.captured(1) : "Profile_1";
            qDebug() << "[ONVIF] ProfileToken:" << profileToken;

            // 获取 RTSP 地址
            QString mediaUrl = serviceAddress;
            mediaUrl.replace("device_service", "media_service");

            QUrl url(mediaUrl);
            QNetworkRequest uriRequest(url);
            uriRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/soap+xml; charset=utf-8");

            QByteArray getUriMsg = QString(
                                       "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                                       "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
                                       "xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\">"
                                       "%1"
                                       "<soap:Body>"
                                       "<trt:GetStreamUri>"
                                       "<trt:StreamSetup>"
                                       "<trt:Stream>RTP-Unicast</trt:Stream>"
                                       "<trt:Transport><trt:Protocol>RTSP</trt:Protocol></trt:Transport>"
                                       "</trt:StreamSetup>"
                                       "<trt:ProfileToken>%2</trt:ProfileToken>"
                                       "</trt:GetStreamUri>"
                                       "</soap:Body>"
                                       "</soap:Envelope>").arg(getWsSecurityHeader()).arg(profileToken).toUtf8();

            QNetworkReply* uriReply = m_impl->networkManager->post(uriRequest, getUriMsg);
            connect(uriReply, &QNetworkReply::finished, this, [this, uriReply, serviceAddress]() {
                if (uriReply->error() == QNetworkReply::NoError) {
                    QString uriRes = QString::fromUtf8(uriReply->readAll());
                    qDebug() << "[ONVIF] GetStreamUri 响应:" << uriRes;

                    QRegularExpression rtspRe("rtsp://[^\\s<]+");
                    auto uriMatch = rtspRe.match(uriRes);
                    if (uriMatch.hasMatch()) {
                        qDebug() << "[ONVIF] 获取 RTSP 地址成功:" << uriMatch.captured();
                        emit streamUriReceived(serviceAddress, uriMatch.captured());
                    }
                } else {
                    qDebug() << "[ONVIF] GetStreamUri 失败:" << uriReply->errorString();
                }
                uriReply->deleteLater();
            });
        } else {
            qDebug() << "[ONVIF] GetProfiles 失败:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

bool OnvifClient::continuousMove(const QString& serviceAddress, double x, double y, double zoom)
{
    QString ptzUrl = serviceAddress;
    ptzUrl.replace("device_service", "ptz_service");

    QUrl url(ptzUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/soap+xml; charset=utf-8");

    // 获取 ProfileToken
    QString profileToken = getFirstProfileToken(serviceAddress);
    if (profileToken.isEmpty()) {
        qDebug() << "[ONVIF] 无法获取 ProfileToken";
        return false;
    }

    // 速度范围限制
    double speedX = qBound(-1.0, x, 1.0);
    double speedY = qBound(-1.0, y, 1.0);
    double speedZoom = qBound(-1.0, zoom, 1.0);

    // 获取认证头（只调用一次）
    QString wsSecurity = getWsSecurityHeader();

    // 构造 SOAP 请求（注意：5个占位符需要5个参数）
    QByteArray msg = QString(
                         "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                         "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
                         "xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" "
                         "xmlns:tt=\"http://www.onvif.org/ver10/schema\">"
                         "%1"  // 占位符1: WS-Security Header
                         "<soap:Body>"
                         "<tptz:ContinuousMove>"
                         "<tptz:ProfileToken>%2</tptz:ProfileToken>"  // 占位符2: ProfileToken
                         "<tptz:Velocity>"
                         "<tt:PanTilt x=\"%3\" y=\"%4\" space=\"http://www.onvif.org/ver10/schema/PTZSpaces/ContinuousGenericSpace\"/>"  // 占位符3,4: x, y
                         "<tt:Zoom x=\"%5\" space=\"http://www.onvif.org/ver10/schema/PTZSpaces/ContinuousGenericSpace\"/>"  // 占位符5: zoom
                         "</tptz:Velocity>"
                         "</tptz:ContinuousMove>"
                         "</soap:Body>"
                         "</soap:Envelope>")
                         .arg(wsSecurity)      // 参数1: Header
                         .arg(profileToken)    // 参数2: ProfileToken
                         .arg(speedX)          // 参数3: x
                         .arg(speedY)          // 参数4: y
                         .arg(speedZoom)       // 参数5: zoom
                         .toUtf8();

    qDebug() << "[ONVIF] PTZ 请求长度:" << msg.size();

    QNetworkReply* reply = m_impl->networkManager->post(request, msg);
    bool success = false;

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QString response = QString::fromUtf8(reply->readAll());
        qDebug() << "[ONVIF] PTZ 响应:" << response.left(500);

        if (!response.contains("<soap:Fault") && !response.contains("<s:Fault")) {
            qDebug() << "[ONVIF] PTZ 控制成功";
            success = true;
        } else {
            qDebug() << "[ONVIF] PTZ 响应包含错误";
        }
    } else {
        qDebug() << "[ONVIF] PTZ 请求失败:" << reply->errorString();
        qDebug() << "HTTP 状态码:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }

    reply->deleteLater();
    return success;
}
bool OnvifClient::stopMove(const QString& serviceAddress)
{
    QString ptzUrl = serviceAddress;
    ptzUrl.replace("device_service", "ptz_service");

    QUrl url(ptzUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/soap+xml; charset=utf-8");

    QString wsSecurity = getWsSecurityHeader();
    QString profileToken = getFirstProfileToken(serviceAddress);
    if (profileToken.isEmpty()) {
        qDebug() << "[ONVIF] 无法获取 ProfileToken";
        return false;
    }

    // 使用专门的 Stop 命令，而不是发送速度为0
    QByteArray msg = QString(
                         "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                         "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
                         "xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\">"
                         "%1"
                         "<soap:Body>"
                         "<tptz:Stop>"
                         "<tptz:ProfileToken>%2</tptz:ProfileToken>"
                         "<tptz:PanTilt>true</tptz:PanTilt>"
                         "<tptz:Zoom>true</tptz:Zoom>"
                         "</tptz:Stop>"
                         "</soap:Body>"
                         "</soap:Envelope>")
                         .arg(wsSecurity)
                         .arg(profileToken)
                         .toUtf8();

    QNetworkReply* reply = m_impl->networkManager->post(request, msg);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool success = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();
    return success;
}
QByteArray OnvifClient::generateNonce()
{
    QByteArray nonce;
    for (int i = 0; i < 16; i++) {
        nonce.append(QRandomGenerator::global()->generate() & 0xFF);
    }
    return nonce;
}

QString OnvifClient::generatePasswordDigest(const QString& password, const QByteArray& nonce, const QString& created)
{
    QByteArray combined = nonce + created.toUtf8() + password.toUtf8();
    QByteArray digest = QCryptographicHash::hash(combined, QCryptographicHash::Sha1);
    return digest.toBase64();
}

QString OnvifClient::getWsSecurityHeader()
{
    if (m_impl->username.isEmpty()) return "";

    QByteArray nonce = generateNonce();
    QString created = QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddThh:mm:ssZ");
    QString passwordDigest = generatePasswordDigest(m_impl->password, nonce, created);

    return QString(
               "<soap:Header>"
               "<wsse:Security xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" "
               "xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">"
               "<wsse:UsernameToken>"
               "<wsse:Username>%1</wsse:Username>"
               "<wsse:Password Type=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest\">%2</wsse:Password>"
               "<wsse:Nonce>%3</wsse:Nonce>"
               "<wsu:Created>%4</wsu:Created>"
               "</wsse:UsernameToken>"
               "</wsse:Security>"
               "</soap:Header>").arg(m_impl->username, passwordDigest, nonce.toBase64(), created);
}

QString OnvifClient::getFirstProfileToken(const QString& serviceAddress)
{
    QString mediaUrl = serviceAddress;
    mediaUrl.replace("device_service", "media_service");

    QUrl url(mediaUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/soap+xml; charset=utf-8");

    QByteArray msg = QString(
                         "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                         "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
                         "xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\">"
                         "%1"
                         "<soap:Body><trt:GetProfiles/></soap:Body>"
                         "</soap:Envelope>").arg(getWsSecurityHeader()).toUtf8();

    QNetworkReply* reply = m_impl->networkManager->post(request, msg);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QString token;
    if (reply->error() == QNetworkReply::NoError) {
        QString res = QString::fromUtf8(reply->readAll());
        QRegularExpression re("token=\"([^\"]+)\"");
        auto match = re.match(res);
        if (match.hasMatch()) {
            token = match.captured(1);
            qDebug() << "[ONVIF] 获取到 ProfileToken:" << token;
        } else {
            qDebug() << "[ONVIF] 未找到 ProfileToken，响应:" << res;
        }
    } else {
        qDebug() << "[ONVIF] GetProfiles 失败:" << reply->errorString();
    }

    reply->deleteLater();
    return token;
}