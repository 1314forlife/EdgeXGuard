#include "rtmp_player.h"
#include "core/common/logger.h"
#include <QElapsedTimer>

// ==================== RenderThread Implementation ====================
RenderThread::RenderThread(RingQueue<FrameData>* queue, OpenGLRenderer* renderer, QObject* parent)
    : QThread(parent)
    , m_queue(queue)
    , m_renderer(renderer)
    , m_running(false)
    , m_interval(33)  // default ~30fps
{
}

RenderThread::~RenderThread()
{
    stop();
}

void RenderThread::stop()
{
    if (m_running) {
        m_running = false;
        wait();
    }
}

void RenderThread::run()
{
    m_running = true;
    LOG_INFO("RenderThread", "========== Render thread started ==========");

    QElapsedTimer frameTimer;
    frameTimer.start();
    int frameCount = 0;
    int emptyQueueCount = 0;

    while (m_running) {
        FrameData frame;
        if (m_queue->pop(frame)) {
            frameCount++;
            if (frameCount % 30 == 0) {
                LOG_INFO("RenderThread", QString("Rendered #%1 frames, queue left: %2")
                             .arg(frameCount).arg(m_queue->size()));
            }

            // ==================== 【★核心修改：改用非阻塞异步队列投递】 ====================
            // 抓取 m_renderer 指针进行异步渲染，渲染线程再也不用死等 UI 线程，各司其职，流畅度直接起飞
            OpenGLRenderer* renderer = m_renderer;
            QMetaObject::invokeMethod(renderer, [renderer, frame]() {
                renderer->updateFrame(frame);
            }, Qt::QueuedConnection);
            // ===========================================================================

            // 帧率常态控制
            int elapsed = frameTimer.elapsed();
            int sleepTime = m_interval - elapsed;
            if (sleepTime > 0) {
                msleep(sleepTime);
            }
            frameTimer.restart();
        } else {
            emptyQueueCount++;
            if (emptyQueueCount % 100 == 0) {
                LOG_DEBUG("RenderThread", QString("Render queue empty, count: %1").arg(emptyQueueCount));
            }
            msleep(1);
        }
    }

    LOG_INFO("RenderThread", QString("========== Render thread exited, total frames: %1 ==========").arg(frameCount));
}

// ==================== RtmpPlayer Implementation ====================
RtmpPlayer::RtmpPlayer(OpenGLRenderer* renderer, QObject* parent)
    : QObject(parent)
    , m_renderer(renderer)
    , m_packetQueue(200)
    , m_frameQueue(100)
    , m_renderThread(nullptr)
{
    connect(&m_demuxer, &Demuxer::sigStreamsReady, this, &RtmpPlayer::onStreamsReady);
    connect(&m_demuxer, &Demuxer::sigError, [](const QString& err) {
        LOG_ERROR("RtmpPlayer", "Demuxer error: " + err);
    });
}

RtmpPlayer::~RtmpPlayer()
{
    close();
}

bool RtmpPlayer::open(const QString& url)
{
    LOG_INFO("RtmpPlayer", QString("Opening URL: %1").arg(url));
    m_demuxer.setPacketQueue(&m_packetQueue);
    return m_demuxer.open(url);
}

void RtmpPlayer::onStreamsReady()
{
    LOG_INFO("RtmpPlayer", "========== Streams ready ==========");
    LOG_INFO("RtmpPlayer", QString("Video size: %1x%2").arg(m_demuxer.width()).arg(m_demuxer.height()));
    LOG_INFO("RtmpPlayer", QString("Video FPS: %1").arg(m_demuxer.fps()));

    // Initialize decoder
    if (!m_decoder.open(m_demuxer.getVideoCodecPar(), m_demuxer.getVideoTimeBase())) {
        LOG_ERROR("RtmpPlayer", "Failed to open decoder");
        return;
    }
    m_decoder.setPacketQueue(&m_packetQueue);
    m_decoder.setOutputQueue(&m_frameQueue);
    m_decoder.start();

    // Calculate render interval
    double fps = m_demuxer.fps();
    if (fps <= 0) fps = 30.0;
    int interval = 1000 / fps;
    LOG_INFO("RtmpPlayer", QString("Target FPS: %1, render interval: %2 ms").arg(fps).arg(interval));

    // Start render thread
    m_renderThread = new RenderThread(&m_frameQueue, m_renderer, this);
    m_renderThread->setInterval(interval);
    m_renderThread->start();

    LOG_INFO("RtmpPlayer", "Render thread started");
}

void RtmpPlayer::start()
{
    LOG_INFO("RtmpPlayer", "Starting demuxer thread");
    m_demuxer.startThread();
}

void RtmpPlayer::close()
{
    LOG_INFO("RtmpPlayer", "Closing player");
    if (m_renderThread) {
        m_renderThread->stop();
        delete m_renderThread;
        m_renderThread = nullptr;
    }
    m_demuxer.stopThread();
    m_decoder.stop();
}