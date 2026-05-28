#ifndef RTMP_PLAYER_H
#define RTMP_PLAYER_H

#include <QObject>
#include <QThread>
#include "core/thread/ring_queue.h"
#include "core/common/packet.h"
#include "core/common/frame.h"
#include "core/decode/demuxer.h"
#include "core/decode/video_decoder.h"
#include "core/render/opengl_renderer.h"

// 渲染线程类
class RenderThread : public QThread
{
    Q_OBJECT
public:
    explicit RenderThread(RingQueue<FrameData>* queue, OpenGLRenderer* renderer, QObject* parent = nullptr);
    ~RenderThread();

    void stop();
    void setInterval(int ms) { m_interval = ms; }

protected:
    void run() override;

private:
    RingQueue<FrameData>* m_queue;
    OpenGLRenderer* m_renderer;
    volatile bool m_running;
    int m_interval;
};

// 播放器主类
class RtmpPlayer : public QObject
{
    Q_OBJECT

public:
    explicit RtmpPlayer(OpenGLRenderer* renderer, QObject* parent = nullptr);
    ~RtmpPlayer();

    bool open(const QString& url);
    void close();
    void start();

private slots:
    void onStreamsReady();

private:
    OpenGLRenderer* m_renderer;

    RingQueue<Packet> m_packetQueue;
    RingQueue<FrameData> m_frameQueue;

    Demuxer m_demuxer;
    VideoDecoder m_decoder;
    RenderThread* m_renderThread;
};

#endif