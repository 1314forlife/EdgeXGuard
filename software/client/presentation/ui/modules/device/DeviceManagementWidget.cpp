#include "DeviceManagementWidget.h"
#include "presentation/viewmodel/device/DeviceViewModel.h"
#include "infrastructure/network/onvif/onvif_client.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSlider>      // 新增
#include <QGridLayout>  // 新增：用于 PTZ 按钮网格布局

DeviceManagementWidget::DeviceManagementWidget(QWidget *parent)
    : QWidget(parent)
    , m_isMoving(false)
    , m_ptzTimer(nullptr)
{
    setupUI();
    loadDevices();
}

void DeviceManagementWidget::onDeviceSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    QString id = item->data(0, Qt::UserRole).toString();
    if (id.isEmpty()) return;

    auto& vm = DeviceViewModel::instance();

    m_nameEdit->setText(vm.getDeviceName(id));
    m_typeEdit->setText(vm.getDeviceType(id));
    m_statusEdit->setText(vm.getDeviceOnline(id) ? "在线" : "离线");
    m_rtspEdit->setText(vm.getRtspUrl(id));
}

void DeviceManagementWidget::setupUI()
{
    // 全局背景调成低调奢侈的现代冷灰色
    this->setStyleSheet(
        "QWidget { background-color: #F8F9FA; font-family: 'Segoe UI', 'Microsoft YaHei'; color: #333333; }"
        "QSplitter::handle { background-color: #E9ECEF; }"
        );

    // 💡 关键点 1：把主布局的四周间距拉满，但取消底部多余的拉伸
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(0); // 布局内部元素基础缝隙清零，靠 margin 控杀

    // 1️⃣ 顶部高级标题栏
    QWidget* headerContainer = new QWidget(this);
    QHBoxLayout* headerLayout = new QHBoxLayout(headerContainer);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* title = new QLabel("设备管理", this);
    // 💡 关键点 2：强行把大标题的底部留白从 20px 限制到 12px
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #1A1D20; letter-spacing: 0.5px; margin-bottom: 12px;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    mainLayout->addWidget(headerContainer);

    // 2️⃣ 核心横向分栏
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setStyleSheet("QSplitter::handle { width: 2px; }");

    // 💡 关键点 3：让主体内容布局（QSplitter）在垂直方向狠狠往上顶
    mainLayout->addWidget(m_splitter, 1); // 这里的 1 代表让它吃掉剩下所有的垂直空间

    // 左侧战区：设备树
    setupDeviceTree();

    // 右侧战区：详情面板
    setupDetailPanel();

    // 比例对齐：左侧占 30%，右侧占 70%
    m_splitter->setSizes(QList<int>({360, 840}));
}

void DeviceManagementWidget::setupDeviceTree()
{
    QWidget* treeContainer = new QWidget(m_splitter);
    QVBoxLayout* treeLayout = new QVBoxLayout(treeContainer);
    treeLayout->setContentsMargins(0, 0, 16, 0);
    treeLayout->setSpacing(12);

    // 列表小标题
    QLabel* treeTitle = new QLabel("设备拓扑列表", treeContainer);
    treeTitle->setStyleSheet("font-weight: 600; font-size: 13px; color: #6C757D; text-transform: uppercase;");
    treeLayout->addWidget(treeTitle);

    // 树控件 QSS 艺术化重构
    m_deviceTree = new QTreeWidget(treeContainer);
    m_deviceTree->setHeaderHidden(true);
    m_deviceTree->setIndentation(24);
    m_deviceTree->setMinimumWidth(280);
    m_deviceTree->setStyleSheet(
        "QTreeWidget {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #E5E5E5;"
        "   border-radius: 8px;"
        "   padding: 8px;"
        "}"
        "QTreeWidget::item {"
        "   padding: 8px 4px;"
        "   margin-bottom: 2px;"
        "   border-radius: 4px;"
        "   color: #495057;"
        "}"
        "QTreeWidget::item:hover {"
        "   background-color: #F1F3F5;"
        "}"
        "QTreeWidget::item:selected {"
        "   background-color: #E8F0FE;"
        "   color: #1A73E8;"
        "   font-weight: 600;"
        "}"
        );
    treeLayout->addWidget(m_deviceTree);

    // 工业风轻量级刷新按钮
    QPushButton* refreshBtn = new QPushButton(" 刷新配置", treeContainer);
    refreshBtn->setFixedHeight(36);
    refreshBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #CED4DA;"
        "   border-radius: 6px;"
        "   color: #495057;"
        "   font-weight: 500;"
        "}"
        "QPushButton:hover { background-color: #F8F9FA; border-color: #B0B5B9; }"
        "QPushButton:pressed { background-color: #E9ECEF; }"
        );
    connect(m_deviceTree, &QTreeWidget::itemClicked, this, &DeviceManagementWidget::onDeviceSelected);
    treeLayout->addWidget(refreshBtn);
    connect(refreshBtn, &QPushButton::clicked, this, &DeviceManagementWidget::loadDevices);

    m_splitter->addWidget(treeContainer);
}

void DeviceManagementWidget::setupDetailPanel()
{
    QWidget* detailContainer = new QWidget(m_splitter);
    QVBoxLayout* detailLayout = new QVBoxLayout(detailContainer);
    detailLayout->setContentsMargins(16, 0, 0, 0);
    detailLayout->setSpacing(12);

    QLabel* detailTitle = new QLabel("设备属性看板", detailContainer);
    detailTitle->setStyleSheet("font-weight: 600; font-size: 13px; color: #6C757D; text-transform: uppercase;");
    detailLayout->addWidget(detailTitle);

    QGroupBox* infoBox = new QGroupBox("配置详情", detailContainer);
    infoBox->setMaximumWidth(550);
    infoBox->setStyleSheet(
        "QGroupBox {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #E5E5E5;"
        "   border-radius: 8px;"
        "   margin-top: 16px;"
        "   padding: 24px 16px 16px 16px;"
        "   font-weight: 600;"
        "   color: #212529;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   subcontrol-position: top left;"
        "   left: 16px;"
        "   padding: 0 4px;"
        "}"
        );

    QFormLayout* formLayout = new QFormLayout(infoBox);
    formLayout->setVerticalSpacing(16);
    formLayout->setHorizontalSpacing(20);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QString inputStyle =
        "QLineEdit {"
        "   border: 1px solid #CED4DA;"
        "   border-radius: 6px;"
        "   padding: 6px 12px;"
        "   background-color: #FFFFFF;"
        "   color: #212529;"
        "}"
        "QLineEdit:focus { border: 1px solid #1A73E8; }"
        "QLineEdit[readOnly=\"true\"] { background-color: #F8F9FA; color: #6C757D; border: 1px dashed #E5E5E5; }";

    // ✅ 改为成员变量赋值
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("未命名边缘节点");
    m_nameEdit->setStyleSheet(inputStyle);

    m_typeEdit = new QLineEdit();
    m_typeEdit->setReadOnly(true);
    m_typeEdit->setStyleSheet(inputStyle);

    m_statusEdit = new QLineEdit();
    m_statusEdit->setReadOnly(true);
    m_statusEdit->setStyleSheet(inputStyle);

    m_rtspEdit = new QLineEdit();
    m_rtspEdit->setReadOnly(true);
    m_rtspEdit->setPlaceholderText("rtsp://192.168.0.102:554/live");
    m_rtspEdit->setStyleSheet(inputStyle);

    formLayout->addRow(new QLabel("设备名称:"), m_nameEdit);
    formLayout->addRow(new QLabel("设备类型:"), m_typeEdit);
    formLayout->addRow(new QLabel("当前状态:"), m_statusEdit);
    formLayout->addRow(new QLabel("RTSP 地址:"), m_rtspEdit);

    detailLayout->addWidget(infoBox);

    QPushButton* saveBtn = new QPushButton("应用更改", detailContainer);
    saveBtn->setFixedWidth(140);
    saveBtn->setFixedHeight(36);
    saveBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #1A73E8;"
        "   border: none;"
        "   border-radius: 6px;"
        "   color: #FFFFFF;"
        "   font-weight: 600;"
        "   font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #1557B0; }"
        "QPushButton:pressed { background-color: #114692; }"
        );
    detailLayout->addWidget(saveBtn);
    // ========== 新增：PTZ 控制面板 ==========
    setupPtzPanel(detailLayout);
    // ======================================
    detailLayout->addStretch();

    m_splitter->addWidget(detailContainer);
}

// ========== PTZ 控制功能 ==========

void DeviceManagementWidget::setupPtzPanel(QVBoxLayout* detailLayout)
{
    QGroupBox* ptzBox = new QGroupBox("云台控制 (PTZ)");
    ptzBox->setMaximumWidth(550);
    ptzBox->setStyleSheet(
        "QGroupBox {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #E5E5E5;"
        "   border-radius: 8px;"
        "   margin-top: 16px;"
        "   padding: 20px 16px 16px 16px;"
        "   font-weight: 600;"
        "   color: #212529;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   subcontrol-position: top left;"
        "   left: 16px;"
        "   padding: 0 4px;"
        "}"
        );

    QVBoxLayout* ptzLayout = new QVBoxLayout(ptzBox);

    // 速度控制行
    QHBoxLayout* speedLayout = new QHBoxLayout();
    QLabel* speedLabel = new QLabel("移动速度:");
    speedLabel->setStyleSheet("font-weight: normal;");
    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(10, 100);  // 10% 到 100%
    m_speedSlider->setValue(30);       // 默认 30%
    m_speedSlider->setFixedWidth(150);
    m_speedSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "   height: 4px;"
        "   background: #E5E5E5;"
        "   border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: #1A73E8;"
        "   width: 14px;"
        "   height: 14px;"
        "   margin: -5px 0;"
        "   border-radius: 7px;"
        "}"
        );
    m_speedLabel = new QLabel("0.3");
    m_speedLabel->setFixedWidth(35);
    m_speedLabel->setStyleSheet("font-weight: normal; color: #1A73E8;");

    speedLayout->addWidget(speedLabel);
    speedLayout->addWidget(m_speedSlider);
    speedLayout->addWidget(m_speedLabel);
    speedLayout->addStretch();

    // 方向按钮网格
    QGridLayout* buttonLayout = new QGridLayout();
    buttonLayout->setSpacing(12);
    buttonLayout->setContentsMargins(30, 10, 30, 10);

    // 创建方向按钮
    QPushButton* btnUp = new QPushButton("↑");
    QPushButton* btnDown = new QPushButton("↓");
    QPushButton* btnLeft = new QPushButton("←");
    QPushButton* btnRight = new QPushButton("→");
    QPushButton* btnStop = new QPushButton("■ 停止");
    QPushButton* btnHome = new QPushButton("● 回中");

    // 按钮样式
    QString btnStyle =
        "QPushButton {"
        "   background-color: #F1F3F5;"
        "   border: 1px solid #DEE2E6;"
        "   border-radius: 6px;"
        "   font-size: 20px;"
        "   font-weight: bold;"
        "   color: #495057;"
        "   min-width: 60px;"
        "   min-height: 50px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #E8F0FE;"
        "   border-color: #1A73E8;"
        "   color: #1A73E8;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #D2E3FC;"
        "}";

    QString stopStyle =
        "QPushButton {"
        "   background-color: #FFF3E0;"
        "   border: 1px solid #FFB74D;"
        "   border-radius: 6px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   color: #E65100;"
        "   min-width: 60px;"
        "   min-height: 50px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #FFE0B2;"
        "}";

    QString homeStyle =
        "QPushButton {"
        "   background-color: #E8F5E9;"
        "   border: 1px solid #66BB6A;"
        "   border-radius: 6px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   color: #2E7D32;"
        "   min-width: 60px;"
        "   min-height: 50px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #C8E6C9;"
        "}";

    btnUp->setStyleSheet(btnStyle);
    btnDown->setStyleSheet(btnStyle);
    btnLeft->setStyleSheet(btnStyle);
    btnRight->setStyleSheet(btnStyle);
    btnStop->setStyleSheet(stopStyle);
    btnHome->setStyleSheet(homeStyle);

    // 布局：上
    buttonLayout->addWidget(btnUp, 0, 1);
    // 布局：左、停止、右
    buttonLayout->addWidget(btnLeft, 1, 0);
    buttonLayout->addWidget(btnStop, 1, 1);
    buttonLayout->addWidget(btnRight, 1, 2);
    // 布局：下、回中
    buttonLayout->addWidget(btnDown, 2, 1);
    buttonLayout->addWidget(btnHome, 3, 1);

    ptzLayout->addLayout(speedLayout);
    ptzLayout->addLayout(buttonLayout);

    detailLayout->addWidget(ptzBox);

    // 连接信号槽
    connect(m_speedSlider, &QSlider::valueChanged, this, &DeviceManagementWidget::onSpeedChanged);

    // 按钮事件：按下时开始移动，松开时停止
    connect(btnUp, &QPushButton::pressed, this, &DeviceManagementWidget::onPtzUp);
    connect(btnUp, &QPushButton::released, this, &DeviceManagementWidget::onPtzStop);
    connect(btnDown, &QPushButton::pressed, this, &DeviceManagementWidget::onPtzDown);
    connect(btnDown, &QPushButton::released, this, &DeviceManagementWidget::onPtzStop);
    connect(btnLeft, &QPushButton::pressed, this, &DeviceManagementWidget::onPtzLeft);
    connect(btnLeft, &QPushButton::released, this, &DeviceManagementWidget::onPtzStop);
    connect(btnRight, &QPushButton::pressed, this, &DeviceManagementWidget::onPtzRight);
    connect(btnRight, &QPushButton::released, this, &DeviceManagementWidget::onPtzStop);
    connect(btnStop, &QPushButton::clicked, this, &DeviceManagementWidget::onPtzStop);
    connect(btnHome, &QPushButton::clicked, this, &DeviceManagementWidget::onPtzHome);

    // 初始化定时器（用于自动停止，已通过按钮 released 实现，暂不需要）
}

void DeviceManagementWidget::onSpeedChanged(int value)
{
    double speed = value / 100.0;
    m_speedLabel->setText(QString::number(speed, 'f', 2));
}

QString DeviceManagementWidget::getCurrentServiceAddress()
{
    QTreeWidgetItem* currentItem = m_deviceTree->currentItem();
    if (!currentItem) {
        qDebug() << "[PTZ] 没有选中任何设备";
        return QString();
    }

    QString deviceId = currentItem->data(0, Qt::UserRole).toString();
    if (deviceId.isEmpty()) {
        qDebug() << "[PTZ] 设备 ID 为空";
        return QString();
    }

    auto& vm = DeviceViewModel::instance();
    QString onvifUrl = vm.getOnvifUrl(deviceId);

    if (onvifUrl.isEmpty()) {
        qDebug() << "[PTZ] 设备没有 ONVIF 地址，设备ID:" << deviceId;
        return QString();
    }

    qDebug() << "[PTZ] 获取到服务地址:" << onvifUrl;
    return onvifUrl;
}

void DeviceManagementWidget::onPtzLeft()
{
    qDebug() << "[PTZ] 向左转";
    QString serviceAddress = getCurrentServiceAddress();
    if (serviceAddress.isEmpty()) {
        qDebug() << "[PTZ] 未选中设备";
        return;
    }

    double speed = m_speedSlider->value() / 100.0;
    OnvifClient onvif;
    onvif.setCredentials("admin", "z13312555");
    onvif.continuousMove(serviceAddress, -speed, 0.0, 0.0);
}

void DeviceManagementWidget::onPtzRight()
{
    qDebug() << "[PTZ] 向右转";
    QString serviceAddress = getCurrentServiceAddress();
    if (serviceAddress.isEmpty()) return;

    double speed = m_speedSlider->value() / 100.0;
    OnvifClient onvif;
    onvif.setCredentials("admin", "z13312555");
    onvif.continuousMove(serviceAddress, speed, 0.0, 0.0);
}

void DeviceManagementWidget::onPtzUp()
{
    qDebug() << "[PTZ] 向上转";
    QString serviceAddress = getCurrentServiceAddress();
    if (serviceAddress.isEmpty()) return;

    double speed = m_speedSlider->value() / 100.0;
    OnvifClient onvif;
    onvif.setCredentials("admin", "z13312555");
    onvif.continuousMove(serviceAddress, 0.0, speed, 0.0);
}

void DeviceManagementWidget::onPtzDown()
{
    qDebug() << "[PTZ] 向下转";
    QString serviceAddress = getCurrentServiceAddress();
    if (serviceAddress.isEmpty()) return;

    double speed = m_speedSlider->value() / 100.0;
    OnvifClient onvif;
    onvif.setCredentials("admin", "z13312555");
    onvif.continuousMove(serviceAddress, 0.0, -speed, 0.0);
}

void DeviceManagementWidget::onPtzStop()
{
    qDebug() << "[PTZ] 停止移动";
    QString serviceAddress = getCurrentServiceAddress();
    if (serviceAddress.isEmpty()) return;

    OnvifClient onvif;
    onvif.setCredentials("admin", "z13312555");
    onvif.stopMove(serviceAddress);
}

void DeviceManagementWidget::onPtzHome()
{
    qDebug() << "[PTZ] 回中位置";
    // 回中功能：需要 AbsoluteMove 或持续移动直到回中
    // 简化实现：先停止，然后发送回中命令（需要根据摄像头能力实现）
    onPtzStop();
    // TODO: 实现 AbsoluteMove 回中功能
}

void DeviceManagementWidget::onPtzMoveTimeout()
{
    // 定时器超时自动停止（备用方案）
    if (m_isMoving) {
        onPtzStop();
        m_isMoving = false;
    }
}

void DeviceManagementWidget::loadDevices()
{
    m_deviceTree->clear();

    auto& vm = DeviceViewModel::instance();
    vm.refresh();

    QVariantList devices = vm.devices();

    // 分组
    QTreeWidgetItem* cameraGroup = new QTreeWidgetItem(m_deviceTree);
    cameraGroup->setText(0, "📹 摄像头");
    cameraGroup->setExpanded(true);

    QTreeWidgetItem* sensorGroup = new QTreeWidgetItem(m_deviceTree);
    sensorGroup->setText(0, "🌡️ 传感器");
    sensorGroup->setExpanded(true);

    QTreeWidgetItem* actuatorGroup = new QTreeWidgetItem(m_deviceTree);
    actuatorGroup->setText(0, "🔌 执行器");
    actuatorGroup->setExpanded(true);

    for (const QVariant& dev : devices) {
        QVariantMap map = dev.toMap();
        QString id = map["id"].toString();
        QString name = map["name"].toString();
        QString type = map["type"].toString();
        bool isOnline = map["isOnline"].toBool();

        QTreeWidgetItem* item = nullptr;
        if (type == "摄像头") {
            item = new QTreeWidgetItem(cameraGroup);
        } else if (type == "传感器") {
            item = new QTreeWidgetItem(sensorGroup);
        } else if (type == "执行器") {
            item = new QTreeWidgetItem(actuatorGroup);
        }

        if (item) {
            item->setText(0, QString("%1 %2").arg(name).arg(isOnline ? "●" : "○"));
            item->setData(0, Qt::UserRole, id);
        }
    }

    m_deviceTree->expandAll();
}