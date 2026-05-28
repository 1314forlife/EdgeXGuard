#ifndef DEMUXER_H
#define DEMUXER_H

#include <QThread>
#include <atomic>
#include "core/common/packet.h"
#include "thread/ring_queue.h"

extern "C" {
#include <libavformat/avformat.h>
}

class Demuxer : public QThread {
    Q_OBJECT
public:
    explicit Demuxer(QObject* parent = nullptr);
    ~Demuxer();

    bool open(const QString& url);
    void close();
    void setPacketQueue(RingQueue<Packet>* queue) { m_packetQueue = queue; }

    void startThread();
    void stopThread();

    int width() const { return m_width; }
    int height() const { return m_height; }
    double fps() const { return m_fps; }
    AVCodecParameters* getVideoCodecPar() const { return m_videoCodecPar; }
    AVRational getVideoTimeBase() const { return m_videoTimeBase; }
    AVFormatContext* getFormatContext() { return m_formatCtx; }
    int getVideoStreamIndex() const { return m_videoStreamIndex; }

signals:
    void sigStreamsReady();
    void sigError(const QString& error);

protected:
    void run() override;

private:
    AVFormatContext* m_formatCtx = nullptr;
    int m_videoStreamIndex = -1;

    AVCodecParameters* m_videoCodecPar = nullptr;
    AVRational m_videoTimeBase{0, 1};

    int m_width = 0;
    int m_height = 0;
    double m_fps = 25.0;

    RingQueue<Packet>* m_packetQueue = nullptr;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_eof{false};
    bool m_isNetworkStream = false;
};

#endif