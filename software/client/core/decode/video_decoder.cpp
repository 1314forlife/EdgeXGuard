#include "video_decoder.h"
#include "core/common/logger.h"
#include <QThread>

extern "C" {
#include <libavformat/avformat.h>
}

VideoDecoder::VideoDecoder(QObject* parent) : QObject(parent) {}

VideoDecoder::~VideoDecoder() {
    close();
}

bool VideoDecoder::open(AVCodecParameters* codecPar, AVRational timeBase) {
    close();

    m_timeBase = timeBase;
    m_width = codecPar->width;
    m_height = codecPar->height;

    LOG_INFO("VideoDecoder", QString("open() - params: %1x%2, timeBase=%3/%4")
                                 .arg(m_width).arg(m_height).arg(timeBase.num).arg(timeBase.den));

    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        emit sigError("Decoder not found");
        return false;
    }

    LOG_INFO("VideoDecoder", QString("Found decoder: %1").arg(codec->name));

    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        emit sigError("Failed to allocate decoder context");
        return false;
    }

    avcodec_parameters_to_context(m_codecCtx, codecPar);
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        emit sigError("Failed to open decoder");
        return false;
    }

    m_frame = av_frame_alloc();
    if (!m_frame) {
        emit sigError("Failed to allocate frame");
        return false;
    }

    m_isOpen = true;
    LOG_INFO("VideoDecoder", QString("Decoder initialized: %1x%2").arg(m_width).arg(m_height));
    return true;
}

void VideoDecoder::close() {
    LOG_INFO("VideoDecoder", "close()");
    m_running = false;
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }
    m_isOpen = false;
}

void VideoDecoder::start() {
    LOG_INFO("VideoDecoder", "start() - creating decoder thread");
    QThread* thread = new QThread;
    connect(thread, &QThread::started, this, &VideoDecoder::run);
    this->moveToThread(thread);
    thread->start();
}

void VideoDecoder::stop() {
    LOG_INFO("VideoDecoder", "stop()");
    m_running = false;
}

void VideoDecoder::run()
{
    LOG_INFO("VideoDecoder", "Decoder thread started");

    if (!m_isOpen || !m_packetQueue || !m_outputQueue) {
        LOG_ERROR("VideoDecoder", "Invalid state or null pointers in run()");
        return;
    }

    m_running = true;
    bool hasKeyFrame = false;
    int totalPackets = 0;
    int totalFrames = 0;
    int skippedPackets = 0;

    while (m_running) {
        Packet packet;
        if (!m_packetQueue->popWait(packet, 10)) {
            continue;
        }

        totalPackets++;
        AVPacket* pkt = packet.get();

        if (totalPackets % 100 == 0) {
            LOG_DEBUG("VideoDecoder", QString("Received packet #%1, queue size: %2")
                          .arg(totalPackets).arg(m_packetQueue->size()));
        }

        if (!hasKeyFrame) {
            if (pkt->flags & AV_PKT_FLAG_KEY) {
                hasKeyFrame = true;
                LOG_INFO("VideoDecoder", QString("Got key frame, start decoding (packet #%1)").arg(totalPackets));
            } else {
                skippedPackets++;
                continue;
            }
        }

        int sendRet = avcodec_send_packet(m_codecCtx, pkt);
        if (sendRet < 0) {
            LOG_WARN("VideoDecoder", "send_packet failed");
            continue;
        }

        while (m_running) {
            int ret = avcodec_receive_frame(m_codecCtx, m_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break; // 需要更多输入包，或者已经解码完毕
            }
            if (ret < 0) {
                LOG_WARN("VideoDecoder", "receive_frame failed");
                break;
            }

            totalFrames++;
            int64_t pts = m_frame->pts;
            if (pts != AV_NOPTS_VALUE) {
                pts = av_rescale_q(pts, m_timeBase, {1, 1000000});
            }

            FrameData frame = FrameData::fromAVFrame(m_frame, pts);

            // 核心修复 2：复用 m_frame 结构体，清除数据缓存引用，解决内存泄漏！
            av_frame_unref(m_frame);

            // 核心修复 3：如果输出队列满了，休眠等待渲染线程消费，绝不丢弃画面！
            while (m_outputQueue->size() >= 30 && m_running) {
                QThread::msleep(10);
            }

            if (m_running) {
                m_outputQueue->push(std::move(frame));
            }
        }
    }

    LOG_INFO("VideoDecoder", QString("Decoder thread exited: totalPackets=%1, skipped=%2, totalFrames=%3")
                                 .arg(totalPackets).arg(skippedPackets).arg(totalFrames));
}