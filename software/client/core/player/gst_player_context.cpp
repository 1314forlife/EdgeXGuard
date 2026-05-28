#include "gst_player_context.h"
#include "core/decode/cpu_software_strategy.h"
#include <QDebug>

GstPlayerContext::GstPlayerContext() {
    // 默认在 PC 上使用软解策略
    m_strategy = std::make_unique<CpuSoftwareStrategy>();
}

GstPlayerContext::~GstPlayerContext() {}

void GstPlayerContext::setStrategy(std::unique_ptr<IDecoderStrategy> strategy) {
    m_strategy = std::move(strategy);
    qDebug() << "[GstPlayerContext] 解码策略已切换为:" << m_strategy->name();
}

void GstPlayerContext::play(const QString& url, uintptr_t winId) {
    QString pipeline = m_strategy->createPipeline(url, winId);
    qDebug() << "[GstPlayerContext] 【打桩】拉起 GStreamer 管道:" << pipeline;
    qDebug() << "[GstPlayerContext] 【打桩】画面将强行注入窗口 WId:" << winId;
}

void GstPlayerContext::stop() {
    qDebug() << "[GstPlayerContext] 【打桩】释放 GStreamer 管道与显存";
}