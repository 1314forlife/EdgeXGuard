#include "MonitoringPage.h"
#include "core/render/opengl_renderer.h"
#include "core/player/rtmp_player.h"
#include <QHBoxLayout>
#include <QLabel>

MonitoringPage::MonitoringPage(QWidget* parent)
    : QWidget(parent)
    , m_renderer(nullptr)
    , m_player(nullptr)
{
    setupUI();
}

MonitoringPage::~MonitoringPage()
{
    if (m_player) {
        m_player->close();
    }
}

void MonitoringPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QLabel* title = new QLabel("实时监控", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; margin-bottom: 20px;");
    mainLayout->addWidget(title);

    // 控制栏
    QHBoxLayout* controlLayout = new QHBoxLayout();
    controlLayout->addWidget(new QLabel("URL:"));
    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText("请输入视频文件路径或RTMP/RTSP地址");
    m_urlEdit->setMinimumWidth(400);
    controlLayout->addWidget(m_urlEdit);

    m_openBtn = new QPushButton("打开", this);
    connect(m_openBtn, &QPushButton::clicked, this, &MonitoringPage::onOpenStream);
    controlLayout->addWidget(m_openBtn);

    m_stopBtn = new QPushButton("停止", this);
    connect(m_stopBtn, &QPushButton::clicked, this, &MonitoringPage::onStopStream);
    controlLayout->addWidget(m_stopBtn);

    mainLayout->addLayout(controlLayout);

    // 视频渲染区域
    m_renderer = new OpenGLRenderer(this);
    m_renderer->setMinimumHeight(400);
    m_renderer->setStyleSheet("background-color: black;");
    mainLayout->addWidget(m_renderer);

    // 状态栏
    m_statusLabel = new QLabel("未连接", this);
    mainLayout->addWidget(m_statusLabel);
}

void MonitoringPage::onOpenStream()
{
    QString url = m_urlEdit->text();
    if (url.isEmpty()) {
        m_statusLabel->setText("请输入URL");
        return;
    }

    m_statusLabel->setText("正在打开...");

    if (m_player) {
        m_player->close();
        delete m_player;
    }

    m_player = new RtmpPlayer(m_renderer, this);
    if (m_player->open(url)) {
        m_player->start();
        m_statusLabel->setText("播放中");
    } else {
        m_statusLabel->setText("打开失败");
    }
}

void MonitoringPage::onStopStream()
{
    if (m_player) {
        m_player->close();
    }
}