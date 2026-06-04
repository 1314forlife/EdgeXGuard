#include "SensorViewModel.h"
#include "application/service/sensor/SensorService.h"
#include "infrastructure/repository_impl/SensorRepository.h"
#include <QTimer>

SensorViewModel& SensorViewModel::instance()
{
    static SensorViewModel vm;
    return vm;
}

SensorViewModel::SensorViewModel()
{
    // 🟢 保持最基础的实例存在，但不再启动任何假数据定时器
    static SensorRepository repo;
    static SensorService service(&repo);

    // 连接 Service 的信号
    QObject::connect(&service, &SensorService::dataUpdated,
                     [this](const SensorData& data) { updateData(data); });

    // ❌ 彻底拔除并注释掉原本每 2 秒无脑刷假数据的 QTimer 恶魔！
    // QTimer* timer = new QTimer();
    // QObject::connect(timer, &QTimer::timeout, [&service]() { service.refreshData(); });
    // timer->start(2000);

    // ❌ 删掉开局立即获取一次的动作，不给假数据露脸的机会
    // service.refreshData();
}

void SensorViewModel::updateData(const SensorData& data)
{
    m_data = data;
    emit dataChanged();
}

QString SensorViewModel::temperature() const
{
    // 如果开机还没收到单片机数据，优雅显示等待状态
    if (m_data.temperature == 0.0f) return "-- °C";
    return QString::number(m_data.temperature, 'f', 1) + " °C";
}

QString SensorViewModel::humidity() const
{
    if (m_data.humidity == 0.0f) return "-- %";
    return QString::number(m_data.humidity, 'f', 0) + " %";
}

QString SensorViewModel::pirStatus() const
{
    return m_data.pirDetected ? "⚠️ 检测到人" : "无人";
}

QString SensorViewModel::updateTime() const
{
    if (!m_data.timestamp.isValid()) return "--:--:--";
    return m_data.timestamp.toString("hh:mm:ss");
}

void SensorViewModel::refresh()
{
    // 🟢 让手动刷新按钮完全失效，或者只做空动作，防止假数据死灰复燃
}

void SensorViewModel::receiveNetworkData(float temperature, float humidity)
{
    // 1. 物理覆盖内部的实体数据（强行接收单片机真数据）
    m_data.temperature = temperature;
    m_data.humidity = humidity;

    // 2. 刷新网关接收到这发网络大炮时的本地绝对时钟
    m_data.timestamp = QDateTime::currentDateTime();

    // 3. 核心：通知所有绑定的 Q_PROPERTY 刷新 View 界面
    emit dataChanged();
}