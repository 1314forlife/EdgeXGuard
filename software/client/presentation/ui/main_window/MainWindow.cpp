#include "MainWindow.h"
#include "presentation/ui/modules/dashboard/DashboardPage.h"
#include "presentation/ui/main_window/SideBar.h"
#include "presentation/viewmodel/sensor/SensorViewModel.h"
#include "presentation/viewmodel/device/DeviceViewModel.h"
#include "presentation/ui/modules/monitor/MonitoringPage.h"
#include "application/service/detection/DetectionService.h"
#include "infrastructure/repository_impl/FaceRepositoryImpl.h"
#include "infrastructure/repository_impl/LinuxPwmServoRepositoryImpl.h"
#include "infrastructure/repository_impl/MqttAlarmRepositoryImpl.h"
#include "presentation/ui/modules/analytics/AnalyticsPage.h"
#include "presentation/ui/modules/face_database/FaceDatabasePage.h"
#include "presentation/ui/modules/perimeter_rules/PerimeterRulesPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFile>
#include <QTimer>

// 模块占位页面（临时，后面替换成真实模块）
class PlaceholderPage : public QWidget
{
public:
    PlaceholderPage(const QString& title, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        QLabel* label = new QLabel(title, this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 20px; color: #888;");
        layout->addWidget(label);
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_sideBar(nullptr)
    , m_stackedWidget(nullptr)
{
    setupUI();
    applyStyle();
    startDataSync();
    setWindowTitle("EdgeXGuard - 边缘智能安防网关");
    resize(1280, 720);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupMenu();
    mainLayout->addWidget(m_sideBar);

    setupCentralArea();
    mainLayout->addWidget(m_stackedWidget, 1);
}

void MainWindow::setupMenu()
{
    m_sideBar = new SideBar(this);
    connect(m_sideBar, &SideBar::menuSelected, this, &MainWindow::onModuleChanged);
}

void MainWindow::setupCentralArea()
{
    m_stackedWidget = new QStackedWidget(this);

    // 1. 重新梳理依赖注入
    auto faceRepo = std::make_shared<FaceRepositoryImpl>();
    auto doorRepo = std::make_shared<LinuxPwmServoRepositoryImpl>();
    auto alarmRepo = std::make_shared<MqttAlarmRepositoryImpl>();

    m_detectionService = std::make_shared<DetectionService>(faceRepo, doorRepo, alarmRepo, this);

    // 2. 更新模块映射关系（剔除温湿度，加入人脸库与规则引擎）
    m_moduleIndexMap = {
        {"dashboard", 0},
        {"monitor", 1},
        {"analytics", 2},
        {"face_database", 3},  // 【★替换温度】人脸凭证库
        {"perimeter_rules", 4},// 【★替换湿度】周界规则引擎
        {"human", 5},
        {"device", 6},
        {"alert", 7},
        {"history", 8},
        {"settings", 9}
    };

    // 3. 重新组装页面序列
    m_stackedWidget->addWidget(new DashboardPage(this));                                    // 0: dashboard
    m_stackedWidget->addWidget(new MonitoringPage(this));                                   // 1: monitor
    m_stackedWidget->addWidget(new AnalyticsPage(m_detectionService, this));                // 2: analytics

    // 替换原本的温湿度占位页
    m_stackedWidget->addWidget(new FaceDatabasePage(this));                                  // 3: face_database
    m_stackedWidget->addWidget(new PerimeterRulesPage(this)); // 4: perimeter_rules
    m_stackedWidget->addWidget(new PlaceholderPage("设备管理 - 配置摄像头、传感器、执行器")); // 6: device
    m_stackedWidget->addWidget(new PlaceholderPage("告警中心 - 历史告警记录"));              // 7: alert
    m_stackedWidget->addWidget(new PlaceholderPage("历史数据 - AI与感知事件历史统计曲线"));   // 8: history
    m_stackedWidget->addWidget(new PlaceholderPage("系统设置 - 用户配置、网络设置、录像计划")); // 9: settings
}

void MainWindow::onModuleChanged(const QString& module)
{
    auto it = m_moduleIndexMap.find(module);
    if (it != m_moduleIndexMap.end()) {
        int index = it.value();
        if (index >= 0 && index < m_stackedWidget->count()) {
            m_stackedWidget->setCurrentIndex(index);
        }
    }
}

void MainWindow::startDataSync()
{
    QTimer* dataTimer = new QTimer(this);
    connect(dataTimer, &QTimer::timeout, this, [this]() {
        updateSideBarData();
    });
    dataTimer->start(1000);
}

void MainWindow::updateSideBarData()
{
    if (!m_sideBar) return;

    auto& sensorVM = SensorViewModel::instance();
    auto& deviceVM = DeviceViewModel::instance();

    // 更新温度
    QString tempStr = sensorVM.temperature();
    bool ok = false;
    float temp = tempStr.left(tempStr.length() - 2).toFloat(&ok);
    if (ok) {
        m_sideBar->updateTemperature(temp);
    }

    // 更新湿度
    QString humiStr = sensorVM.humidity();
    int humi = humiStr.left(humiStr.length() - 1).toInt(&ok);
    if (ok) {
        m_sideBar->updateHumidity(humi);
    }

    // 更新人体检测
    bool humanDetected = sensorVM.pirStatus().contains("检测到人");
    m_sideBar->updateHumanDetected(humanDetected);

    // 更新设备状态
    for (const QVariant& dev : deviceVM.devices()) {
        QVariantMap map = dev.toMap();
        QString id = map["id"].toString();
        bool isOn = map["isOn"].toBool();

        if (id == "fan_1") {
            m_sideBar->updateFanStatus(isOn);
        } else if (id == "light_1") {
            m_sideBar->updateLightStatus(isOn);
        }
    }
}

void MainWindow::applyStyle()
{
    QFile styleFile(":/resources/styles.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        setStyleSheet(style);
    }
}