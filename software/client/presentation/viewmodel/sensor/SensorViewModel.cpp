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
    static SensorRepository repo;
    static SensorService service(&repo);

    // 连接 Service 的信号
    QObject::connect(&service, &SensorService::dataUpdated,
                     [this](const SensorData& data) { updateData(data); });

    // 启动定时器，每2秒刷新一次
    QTimer* timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [&service]() { service.refreshData(); });
    timer->start(2000);

    // 立即获取一次数据
    service.refreshData();
}

void SensorViewModel::updateData(const SensorData& data)
{
    m_data = data;
    emit dataChanged();
}

QString SensorViewModel::temperature() const
{
    return QString::number(m_data.temperature, 'f', 1) + " °C";
}

QString SensorViewModel::humidity() const
{
    return QString::number(m_data.humidity, 'f', 0) + " %";
}

QString SensorViewModel::pirStatus() const
{
    return m_data.pirDetected ? "⚠️ 检测到人" : "无人";
}

QString SensorViewModel::updateTime() const
{
    return m_data.timestamp.toString("hh:mm:ss");
}

void SensorViewModel::refresh()
{
    // 手动刷新
    static SensorRepository repo;
    static SensorService service(&repo);
    service.refreshData();
}