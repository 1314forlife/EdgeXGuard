#include <QApplication>
#include "presentation/ui/main_window/MainWindow.h"
#include "infrastructure/network/mqtt/mqtt_network_service.h"
#include "presentation/viewmodel/sensor/SensorViewModel.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 1. 初始化主窗口 UI
    MainWindow w;
    w.show();

    // 2. 实例化自研 MQTT 核心 network 层
    MqttNetworkService* mqttService = new MqttNetworkService(&a);

    // 3. 跨模块高压电缆：把网络层抓到的温湿度，直接物理灌入 UI 核心（ViewModel）
    QObject::connect(mqttService, &MqttNetworkService::sensorDataUpdated,
                     &SensorViewModel::instance(), &SensorViewModel::receiveNetworkData);

    // 4. 带上管理员账号密码，正式推开 Rock 5T 门禁大门
    mqttService->connectToGateway("192.168.0.102", 1883);

    return a.exec();
}