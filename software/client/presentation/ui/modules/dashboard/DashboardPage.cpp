#include "DashboardPage.h"
#include "presentation/viewmodel/sensor/SensorViewModel.h"
#include "presentation/viewmodel/device/DeviceViewModel.h"
#include "presentation/ui/common/ChartWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <QScrollArea>
#include <QDebug>

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
    , m_maxHistoryPoints(60)
{
    setupUI();

    // ❌ 彻底摘除原本的 QTimer 轮询假定时器

    // 🟢 改为事件驱动：当网络层改写 ViewModel 时，立刻引爆 UI 重绘
    connect(&SensorViewModel::instance(), &SensorViewModel::dataChanged,
            this, &DashboardPage::updateSensorDisplay, Qt::UniqueConnection);

    updateSensorDisplay();
}

void DashboardPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    mainLayout->setSpacing(20);

    QLabel* title = new QLabel("仪表板", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold;");
    mainLayout->addWidget(title);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget* contentWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(20);

    // 传感器卡片布局
    QHBoxLayout* sensorLayout = new QHBoxLayout();
    sensorLayout->setSpacing(20);

    QGroupBox* tempBox = new QGroupBox("温度");
    tempBox->setStyleSheet("QGroupBox { font-weight: bold; }");
    QVBoxLayout* tempLayout = new QVBoxLayout(tempBox);
    m_tempLabel = new QLabel("-- °C");
    m_tempLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #4A90E2;");
    tempLayout->addWidget(m_tempLabel);
    sensorLayout->addWidget(tempBox);

    QGroupBox* humidBox = new QGroupBox("湿度");
    QVBoxLayout* humidLayout = new QVBoxLayout(humidBox);
    m_humidLabel = new QLabel("-- %");
    m_humidLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #4CAF50;");
    humidLayout->addWidget(m_humidLabel);
    sensorLayout->addWidget(humidBox);

    QGroupBox* pirBox = new QGroupBox("人体检测");
    QVBoxLayout* pirLayout = new QVBoxLayout(pirBox);
    m_pirLabel = new QLabel("无人");
    m_pirLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #888;");
    pirLayout->addWidget(m_pirLabel);
    sensorLayout->addWidget(pirBox);

    sensorLayout->setStretch(0, 1);
    sensorLayout->setStretch(1, 1);
    sensorLayout->setStretch(2, 1);
    contentLayout->addLayout(sensorLayout);

    // 创建自研图表并注入
    createChartCard();
    contentLayout->addWidget(m_chartWidget);

    // 下方元器件运行状态
    QGroupBox* deviceBox = new QGroupBox("设备状态");
    QGridLayout* deviceLayout = new QGridLayout(deviceBox);

    deviceLayout->addWidget(new QLabel("风扇:"), 0, 0);
    m_fanLabel = new QLabel("关闭");
    m_fanLabel->setStyleSheet("color: #F44336;");
    deviceLayout->addWidget(m_fanLabel, 0, 1);

    deviceLayout->addWidget(new QLabel("灯光:"), 1, 0);
    m_lightLabel = new QLabel("关闭");
    m_lightLabel->setStyleSheet("color: #F44336;");
    deviceLayout->addWidget(m_lightLabel, 1, 1);

    deviceLayout->setColumnStretch(2, 1);
    contentLayout->addWidget(deviceBox);
    contentLayout->addStretch();

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
}

void DashboardPage::createChartCard()
{
    m_chartWidget = new ChartWidget(this);
    m_chartWidget->setMinimumHeight(280);
    m_chartWidget->setMaxDataPoints(60);
    m_chartWidget->showTemperature(true);
    m_chartWidget->showHumidity(true);
    m_chartWidget->setTitle("温湿度历史曲线（最近60秒）");
}

void DashboardPage::updateSensorDisplay()
{
    auto& sensorVM = SensorViewModel::instance();
    auto& deviceVM = DeviceViewModel::instance();

    // 1. 同步刷新顶部看板卡片的文本
    QString tempStr = sensorVM.temperature();
    m_tempLabel->setText(tempStr);

    QString humiStr = sensorVM.humidity();
    m_humidLabel->setText(humiStr);

    QString pir = sensorVM.pirStatus();
    m_pirLabel->setText(pir.contains("检测到人") ? "检测到人" : "无人");
    if (pir.contains("检测到人")) {
        m_pirLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #F44336;");
    } else {
        m_pirLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #888;");
    }

    // 2. 🟢 剥离纯数值并打入历史波形队列
    QString pureTemp = tempStr.toUpper().replace("°C", "").trimmed();
    QString pureHumi = humiStr.replace("%", "").trimmed();

    float temp = pureTemp.toFloat();
    int humi   = pureHumi.toInt();

    // 只有当真正接收到物理世界的差异新跳变点时才入队，防止重连空数据冲洗画布
    if (m_tempHistory.isEmpty() || m_tempHistory.last() != temp || m_humiHistory.last() != humi) {
        m_tempHistory.append(temp);
        m_humiHistory.append(humi);

        // 维持 60 点滑动时间窗口
        while (m_tempHistory.size() > m_maxHistoryPoints) {
            m_tempHistory.removeFirst();
            m_humiHistory.removeFirst();
        }

        // 物理推入折线图画布进行底层重绘
        if (m_chartWidget && !m_tempHistory.isEmpty()) {
            m_chartWidget->setTemperatureData(m_tempHistory);
            m_chartWidget->setHumidityData(m_humiHistory);
        }
        qDebug() << "[UI Chart] 📈 真实物理波形更新成功 -> 温度:" << temp << " 湿度:" << humi;
    }

    // 3. 更新下方的风扇、灯光元器件运行状态
    QVariantList devices = deviceVM.devices();
    for (const QVariant& dev : devices) {
        QVariantMap map = dev.toMap();
        QString id = map["id"].toString();
        bool isOn = map["isOn"].toBool();

        if (id == "fan_1") {
            m_fanLabel->setText(isOn ? "开启" : "关闭");
            m_fanLabel->setStyleSheet(isOn ? "color: #4CAF50;" : "color: #F44336;");
        } else if (id == "light_1") {
            m_lightLabel->setText(isOn ? "开启" : "关闭");
            m_lightLabel->setStyleSheet(isOn ? "color: #4CAF50;" : "color: #F44336;");
        }
    }
}