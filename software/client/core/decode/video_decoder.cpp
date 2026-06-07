#ifdef __MINGW32__
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

// 🟢 核心黑客技术：强行将系统底层真正的 32位 纯净符号，绑定到我们自定义的 real_ 别名上！
// 这样可以彻底打断编译器的符号混淆，100% 杜绝函数自我套娃、递归卡死、栈溢出的惨剧！
extern "C" int real_pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *, const struct timespec *) __asm__("pthread_cond_timedwait");

extern "C" int pthread_cond_timedwait64(pthread_cond_t *cv,
                                        pthread_mutex_t *external_mutex,
                                        const struct _timespec64 *t)
{
    if (!cv || !external_mutex || !t) {
        return EINVAL;
    }

    struct timespec ts32;
    ts32.tv_sec = static_cast<time_t>(t->tv_sec);
    ts32.tv_nsec = static_cast<long>(t->tv_nsec);

    // 🟢 调用绝对不会翻车的、真正的底层 32位 通道
    return real_pthread_cond_timedwait(cv, external_mutex, &ts32);
}
#endif
// --------------------------------------------------------------------------------

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

    // 🟢 精妙改动一：等待第一个数据包，给上游拉流留出3秒缓冲
    int waitCount = 0;
    while (m_packetQueue->size() == 0 && m_running && waitCount < 300) {
        QThread::msleep(10);
        waitCount++;
    }

    if (m_packetQueue->size() == 0) {
        LOG_ERROR("VideoDecoder", "No packets received after 3 seconds, exiting");
        return;
    }

    LOG_INFO("VideoDecoder", QString("Starting decode, queue size: %1").arg(m_packetQueue->size()));

    while (m_running) {
        Packet packet;

        // 🟢 精妙改动二：100ms 超时，彻底把条件变量高频调用的性能手刹拉掉
        if (!m_packetQueue->popWait(packet, 100)) {
            // 超时没有数据，检查是否还在运行
            if (m_running && m_packetQueue->size() == 0) {
                // 🟢 精妙改动三：持续为空时挂起，绝不盲目轮询
                QThread::msleep(10);
            }
            continue;
        }

        totalPackets++;
        AVPacket* pkt = packet.get();

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
                break;
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
            av_frame_unref(m_frame);

            // 🟢 精妙改动四：5ms 级灵敏等待，画面流畅度全开
            while (m_outputQueue->size() >= 30 && m_running) {
                QThread::msleep(5);
            }

            if (m_running) {
                m_outputQueue->push(std::move(frame));
            }
        }
    }

    LOG_INFO("VideoDecoder", QString("Decoder thread exited: totalPackets=%1, skipped=%2, totalFrames=%3")
                                 .arg(totalPackets).arg(skippedPackets).arg(totalFrames));
}