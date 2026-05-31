#ifndef ONVIF_CLIENT_H
#define ONVIF_CLIENT_H

#include <QObject>
#include <QString>
#include <QList>

struct OnvifDeviceInfo {
    QString serviceAddress;   // 设备服务地址，如 http://192.168.1.100:80/onvif/device_service
    QString hardware;          // 硬件型号
    QString firmware;          // 固件版本
    QString serialNumber;      // 序列号
    QString manufacturer;      // 制造商
};

class OnvifClient : public QObject
{
    Q_OBJECT

public:
    explicit OnvifClient(QObject *parent = nullptr);
    ~OnvifClient();

    // 设备发现：搜索局域网内所有 ONVIF 摄像头
    QList<OnvifDeviceInfo> discoverDevices(int timeoutMs = 3000);

    // 连接到指定设备
    bool connectToDevice(const QString& serviceAddress, const QString& username, const QString& password);

    // 获取设备信息
    OnvifDeviceInfo getDeviceInfo();

    // 获取 RTSP 流地址（主码流）
    QString getStreamUri();

    // PTZ 控制
    bool continuousMove(double x, double y, double zoom);
    bool stopMove();
    bool gotoPreset(const QString& presetToken);

private:
    class Impl;
    Impl* m_impl;
};

#endif // ONVIF_CLIENT_H