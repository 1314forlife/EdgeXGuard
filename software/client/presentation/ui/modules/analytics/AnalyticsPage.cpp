#include "AnalyticsPage.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDateTime>

AnalyticsPage::AnalyticsPage(std::shared_ptr<DetectionService> detectionService, QWidget* parent)
    : QWidget(parent)
    , m_detectionService(detectionService)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 标题
    QLabel* titleLabel = new QLabel("智能分析与边缘人证核验终端", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #f8fafc;");
    mainLayout->addWidget(titleLabel);

    // 模拟刷脸按钮
    m_mockVerifyBtn = new QPushButton("点击模拟实时刷脸验证", this);
    m_mockVerifyBtn->setMinimumHeight(44);
    m_mockVerifyBtn->setCursor(Qt::PointingHandCursor);
    m_mockVerifyBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3b82f6;
            color: white;
            font-size: 14px;
            font-weight: 600;
            border: none;
            border-radius: 8px;
        }
        QPushButton:hover { background-color: #2563eb; }
        QPushButton:pressed { background-color: #1d4ed8; }
    )");
    mainLayout->addWidget(m_mockVerifyBtn);

    // 黑绿风格控制台
    m_logConsole = new QTextEdit(this);
    m_logConsole->setReadOnly(true);
    m_logConsole->setStyleSheet(R"(
        QTextEdit {
            background-color: #0f172a;
            color: #38bdf8;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 13px;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 10px;
        }
    )");
    mainLayout->addWidget(m_logConsole, 1);

    // 绑定按钮点击事件
    connect(m_mockVerifyBtn, &QPushButton::clicked, this, &AnalyticsPage::onMockVerifyClicked);

    // 🔗 核心纽带：将Service层的核心信号绑定到UI刷新槽函数上
    if (m_detectionService) {
        connect(m_detectionService.get(), &DetectionService::doorOpened, this, &AnalyticsPage::onDoorOpened);
        connect(m_detectionService.get(), &DetectionService::strangerAlertTriggered, this, &AnalyticsPage::onStrangerAlert);
    }
}

void AnalyticsPage::onMockVerifyClicked() {
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logConsole->append(QString("[%1] [UI层] -> 用户触发刷脸，正在提取512维特征值并上报Service...").arg(timeStr));

    // 伪造一组512维人脸向量数据扔给Service
    std::vector<float> mockFeature(512, 0.25f);
    if (m_detectionService) {
        m_detectionService->processFaceVerification(mockFeature);
    }
}

void AnalyticsPage::onDoorOpened(const QString& name) {
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logConsole->append(QString("<font color='#4ade80'>[%1] 【业务反馈】比对通过！识别到登记用户【%2】，网关已驱动本地硬件PWM舵机，执行开门动作！</font>")
                             .arg(timeStr).arg(name));
}

void AnalyticsPage::onStrangerAlert() {
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logConsole->append(QString("<font color='#f87171'>[%1] 【安全警报】比对失败！检测到未登记的陌生人，网关已向分布式MQTT总线发布强闯指令，驱使远端ESP8266长鸣报警！</font>")
                             .arg(timeStr));
}