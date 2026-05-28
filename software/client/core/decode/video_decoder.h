#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <QObject>
#include <atomic>
#include "core/common/frame.h"
#include "core/common/packet.h"
#include "thread/ring_queue.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

class VideoDecoder : public QObject {
    Q_OBJECT
public:
    explicit VideoDecoder(QObject* parent = nullptr);
    ~VideoDecoder();

    bool open(AVCodecParameters* codecPar, AVRational timeBase);
    void close();

    void setPacketQueue(RingQueue<Packet>* queue) { m_packetQueue = queue; }
    void setOutputQueue(RingQueue<FrameData>* queue) { m_outputQueue = queue; }

    void stop();
    void start();  // 添加 start 方法

    int width() const { return m_width; }
    int height() const { return m_height; }
    AVCodecContext* getCodecContext() { return m_codecCtx; }
signals:
    void sigError(const QString& error);

private slots:
    void run();  // 改为 private slots

private:
    AVCodecContext* m_codecCtx = nullptr;
    AVFrame* m_frame = nullptr;

    int m_width = 0;
    int m_height = 0;
    AVRational m_timeBase{1, 1000000};

    RingQueue<Packet>* m_packetQueue = nullptr;
    RingQueue<FrameData>* m_outputQueue = nullptr;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_isOpen{false};
    std::atomic<bool> m_hasKeyFrame{false};
};

#endif