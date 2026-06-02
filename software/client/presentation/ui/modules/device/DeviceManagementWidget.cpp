#include "DeviceManagementWidget.h"
#include "presentation/viewmodel/device/DeviceViewModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>

DeviceManagementWidget::DeviceManagementWidget(QWidget *parent)
    : QWidget(parent)
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
    detailLayout->addStretch();

    m_splitter->addWidget(detailContainer);
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