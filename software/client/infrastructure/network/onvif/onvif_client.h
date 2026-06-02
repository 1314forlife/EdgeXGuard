#ifndef ONVIF_CLIENT_H
#define ONVIF_CLIENT_H

#include <QObject>
#include <QString>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>

struct OnvifDeviceInfo {
    QString ipAddress;
    QString serviceAddress;
    QString manufacturer;
    QString hardware;
    QString firmware;
    QString serialNumber;
};

class OnvifClient : public QObject
{
    Q_OBJECT

public:
    explicit OnvifClient(QObject *parent = nullptr);
    ~OnvifClient();

    void setCredentials(const QString& username, const QString& password);
    void discoverDevices();
    void requestDeviceInfo(const QString& serviceAddress);
    void requestStreamUri(const QString& serviceAddress);

    // PTZ 控制
    bool continuousMove(const QString& serviceAddress, double x, double y, double zoom);
    bool stopMove(const QString& serviceAddress);

    // 获取 ProfileToken（用于调试和测试）
    QString getFirstProfileToken(const QString& serviceAddress);

signals:
    void deviceDiscovered(const OnvifDeviceInfo& dev);
    void deviceInfoReceived(const QString& serviceAddress, const OnvifDeviceInfo& info);
    void streamUriReceived(const QString& serviceAddress, const QString& rtspUrl);

private:
    QString generatePasswordDigest(const QString& password, const QByteArray& nonce, const QString& created);
    QByteArray generateNonce();
    QString getWsSecurityHeader();

    class Impl;
    Impl* m_impl;
};

#endif // ONVIF_CLIENT_H