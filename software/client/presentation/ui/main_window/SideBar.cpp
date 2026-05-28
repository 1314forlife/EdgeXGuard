#include "SideBar.h"
#include <QScrollArea>
#include <QFrame>

SideBar::SideBar(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void SideBar::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ========== 1. Logo 区域 ==========
    QWidget *logoWidget = new QWidget(this);
    logoWidget->setFixedHeight(100);
    QVBoxLayout *logoLayout = new QVBoxLayout(logoWidget);
    logoLayout->setContentsMargins(20, 24, 20, 16);

    QLabel *logoTitle = new QLabel("EdgeXGuard");
    logoTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: white;");

    QLabel *logoSub = new QLabel("边缘智能安防网关");
    logoSub->setStyleSheet("font-size: 11px; color: #94a3b8;");

    logoLayout->addWidget(logoTitle);
    logoLayout->addWidget(logoSub);
    logoLayout->addStretch();

    // ========== 2. 菜单区域 ==========
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget *menuContainer = new QWidget();
    QVBoxLayout *menuLayout = new QVBoxLayout(menuContainer);
    menuLayout->setContentsMargins(12, 16, 12, 20);
    menuLayout->setSpacing(4);

    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true);

    // 菜单列表
    QList<QPair<QString, QString>> menus = {
        {"仪表板", "dashboard"},
        {"实时监控", "monitor"},
        {"智能分析", "analytics"},
        {"人脸凭证管理", "face_database"},    // 【★替换温度】
        {"周界规则引擎", "perimeter_rules"},  // 【★替换湿度】
        {"人体检测", "human"},
        {"设备管理", "device"},
        {"告警中心", "alert"},
        {"历史数据", "history"},
        {"系统设置", "settings"}
    };

    for (int i = 0; i < menus.size(); ++i) {
        const auto &menu = menus[i];
        QPushButton *btn = new QPushButton(menu.first);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(44);
        btn->setProperty("module", menu.second);

        btn->setStyleSheet(R"(
            QPushButton {
                text-align: left;
                padding-left: 20px;
                background-color: transparent;
                color: #94a3b8;
                font-size: 14px;
                font-weight: 500;
                border: none;
                border-radius: 10px;
            }
            QPushButton:hover {
                background-color: #1e293b;
                color: #f1f5f9;
            }
            QPushButton:checked {
                background-color: #3b82f6;
                color: white;
            }
        )");

        if (i == 0) btn->setChecked(true);

        m_buttonGroup->addButton(btn, i);

        connect(btn, &QPushButton::clicked, this, [this, module = menu.second]() {
            emit menuSelected(module);
        });

        menuLayout->addWidget(btn);
    }

    menuLayout->addStretch();  // 关键：把菜单推上去，消除底部留白
    scrollArea->setWidget(menuContainer);

    // ========== 3. 底部状态区域 ==========
    QWidget *bottomWidget = new QWidget(this);
    bottomWidget->setFixedHeight(160);
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(16, 16, 16, 20);
    bottomLayout->setSpacing(10);

    // 第一行：温度、湿度、人体
    QHBoxLayout *row1 = new QHBoxLayout();
    row1->setSpacing(12);

    m_tempLabel = new QLabel("🌡️ --°C");
    m_tempLabel->setStyleSheet("color: #facc15; font-size: 13px; font-weight: 600;");

    m_humiLabel = new QLabel("💧 --%");
    m_humiLabel->setStyleSheet("color: #60a5fa; font-size: 13px; font-weight: 600;");

    m_humanLabel = new QLabel("🚶 无人");
    m_humanLabel->setStyleSheet("color: #94a3b8; font-size: 12px;");

    row1->addWidget(m_tempLabel);
    row1->addWidget(m_humiLabel);
    row1->addStretch();
    row1->addWidget(m_humanLabel);

    // 分割线
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #334155; max-height: 1px;");

    // 设备状态
    QLabel *deviceTitle = new QLabel("设备状态");
    deviceTitle->setStyleSheet("color: #94a3b8; font-size: 11px; letter-spacing: 1px;");

    QHBoxLayout *row2 = new QHBoxLayout();
    row2->setSpacing(20);

    m_fanLabel = new QLabel("风扇：关闭");
    m_lightLabel = new QLabel("灯光：关闭");
    m_fanLabel->setStyleSheet("color: #f97316; font-size: 12px;");
    m_lightLabel->setStyleSheet("color: #f97316; font-size: 12px;");

    row2->addWidget(m_fanLabel);
    row2->addStretch();
    row2->addWidget(m_lightLabel);

    bottomLayout->addLayout(row1);
    bottomLayout->addWidget(line);
    bottomLayout->addWidget(deviceTitle);
    bottomLayout->addLayout(row2);

    bottomWidget->setStyleSheet(
        "QWidget { background-color: #0f172a; border-top: 1px solid #1e293b; }"
        );

    // ========== 4. 组装 ==========
    mainLayout->addWidget(logoWidget);
    mainLayout->addWidget(scrollArea, 1);
    mainLayout->addWidget(bottomWidget);

    setStyleSheet("SideBar { background-color: #0f172a; }");
    setFixedWidth(260);
}

// ========== 数据更新接口 ==========
void SideBar::updateTemperature(float temp) {
    m_tempLabel->setText(QString("🌡️ %1°C").arg(temp, 0, 'f', 1));
}

void SideBar::updateHumidity(int humi) {
    m_humiLabel->setText(QString("💧 %1%").arg(humi));
}

void SideBar::updateHumanDetected(bool detected) {
    m_humanLabel->setText(detected ? "🚶 有人" : "🚶 无人");
    m_humanLabel->setStyleSheet(detected ?
                                    "color: #facc15; font-size: 12px;" :
                                    "color: #94a3b8; font-size: 12px;");
}

void SideBar::updateFanStatus(bool on) {
    m_fanLabel->setText(on ? "风扇：开启" : "风扇：关闭");
    m_fanLabel->setStyleSheet(on ?
                                  "color: #22c55e; font-size: 12px;" :
                                  "color: #f97316; font-size: 12px;");
}

void SideBar::updateLightStatus(bool on) {
    m_lightLabel->setText(on ? "灯光：开启" : "灯光：关闭");
    m_lightLabel->setStyleSheet(on ?
                                    "color: #22c55e; font-size: 12px;" :
                                    "color: #f97316; font-size: 12px;");
}