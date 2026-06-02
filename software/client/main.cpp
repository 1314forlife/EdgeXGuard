#include <QApplication>
#include <QTimer>
#include <QDebug>
#include "presentation/ui/main_window/MainWindow.h"
#include "infrastructure/network/onvif/onvif_client.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    OnvifClient onvif;
    onvif.setCredentials("admin", "z13312555");

    // 保存服务地址供后续使用
    QString savedServiceAddress;

    // 1. 发现设备
    QObject::connect(&onvif, &OnvifClient::deviceDiscovered,
                     [&onvif, &savedServiceAddress](const OnvifDeviceInfo& dev) {
                         qDebug() << "发现设备:" << dev.ipAddress;
                         qDebug() << "服务地址:" << dev.serviceAddress;
                         savedServiceAddress = dev.serviceAddress;

                         // 获取设备详细信息
                         onvif.requestDeviceInfo(dev.serviceAddress);
                     });

    // 2. 接收设备详细信息 + 获取 RTSP 地址
    QObject::connect(&onvif, &OnvifClient::deviceInfoReceived,
                     [&onvif](const QString& serviceAddress, const OnvifDeviceInfo& info) {
                         qDebug() << "========== 设备详细信息 ==========";
                         qDebug() << "制造商:" << info.manufacturer;
                         qDebug() << "硬件型号:" << info.hardware;
                         qDebug() << "固件版本:" << info.firmware;
                         qDebug() << "序列号:" << info.serialNumber;
                         qDebug() << "=================================";

                         // 获取 RTSP 地址
                         onvif.requestStreamUri(serviceAddress);
                     });

    // 3. 接收 RTSP 地址后，开始 PTZ 测试
    QObject::connect(&onvif, &OnvifClient::streamUriReceived,
                     [&onvif](const QString& serviceAddress, const QString& rtspUrl) {
                         qDebug() << "========== RTSP 地址 ==========";
                         qDebug() << "RTSP 地址:" << rtspUrl;
                         qDebug() << "===============================";

                         // PTZ 测试（只执行一次）
                         static bool ptzTested = false;
                         if (!ptzTested) {
                             ptzTested = true;

                             // 延迟2秒，确保设备准备好
                             QTimer::singleShot(2000, [&onvif, serviceAddress]() {
                                 qDebug() << "\n========== 开始 PTZ 测试 ==========";

                                 // 获取 ProfileToken
                                 QString profileToken = onvif.getFirstProfileToken(serviceAddress);
                                 qDebug() << "获取到的 ProfileToken:" << profileToken;

                                 if (profileToken.isEmpty()) {
                                     qDebug() << "❌ 无法获取 ProfileToken，摄像头可能不支持 PTZ";
                                     return;
                                 }

                                 // 测试1：向右转 2 秒
                                 qDebug() << "测试1: 向右转 (速度 0.3)";
                                 bool result = onvif.continuousMove(serviceAddress, 0.3, 0.0, 0.0);
                                 qDebug() << "PTZ 命令返回:" << (result ? "成功" : "失败");

                                 // 2秒后停止
                                 QTimer::singleShot(2000, [&onvif, serviceAddress]() {
                                     qDebug() << "停止移动";
                                     onvif.stopMove(serviceAddress);
                                 });

                                 // 测试2：向上转（延迟5秒后执行）
                                 QTimer::singleShot(5000, [&onvif, serviceAddress]() {
                                     qDebug() << "\n测试2: 向上转 (速度 0.3)";
                                     bool result = onvif.continuousMove(serviceAddress, 0.0, 0.3, 0.0);
                                     qDebug() << "PTZ 命令返回:" << (result ? "成功" : "失败");

                                     QTimer::singleShot(2000, [&onvif, serviceAddress]() {
                                         qDebug() << "停止移动";
                                         onvif.stopMove(serviceAddress);
                                     });
                                 });

                                 // 测试3：放大（延迟9秒后执行）
                                 QTimer::singleShot(9000, [&onvif, serviceAddress]() {
                                     qDebug() << "\n测试3: 向左转 (速度 0.3)";
                                     bool result = onvif.continuousMove(serviceAddress, -0.3, 0.0, 0.0);
                                     qDebug() << "PTZ 命令返回:" << (result ? "成功" : "失败");

                                     QTimer::singleShot(2000, [&onvif, serviceAddress]() {
                                         qDebug() << "停止移动";
                                         onvif.stopMove(serviceAddress);
                                         qDebug() << "========== PTZ 测试完成 ==========\n";
                                     });
                                 });
                             });
                         }
                     });

    qDebug() << "开始搜索 ONVIF 设备...";
    onvif.discoverDevices();

    MainWindow window;
    window.show();

    return app.exec();
}