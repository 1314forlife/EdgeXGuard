#include "demuxer.h"
#include "core/common/logger.h"
#include <QDebug>

Demuxer::Demuxer(QObject* parent) : QThread(parent) {
    static bool init = false;
    if (!init) {
        avformat_network_init();
        init = true;
    }
}

Demuxer::~Demuxer() {
    stopThread();
    close();
}

bool Demuxer::open(const QString& url) {
    close();

    LOG_INFO("Demuxer", QString("open() - URL: %1").arg(url));

    // ==================== 【★大招：扒光所有参数，拒绝画蛇添足】 ====================
    // 直接传 nullptr，让 FFmpeg 自动根据本地网络环境用最兼容的方式去握手打开
    if (avformat_open_input(&m_formatCtx, url.toStdString().c_str(), nullptr, nullptr) < 0) {
        LOG_ERROR("Demuxer", "Cannot open file/stream");
        emit sigError("Cannot open file/stream");
        return false;
    }
    // =========================================================================

    LOG_INFO("Demuxer", "avformat_open_input success");

    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        LOG_ERROR("Demuxer", "Cannot find stream info");
        emit sigError("Cannot find stream info");
        return false;
    }

    LOG_INFO("Demuxer", QString("Stream count: %1").arg(m_formatCtx->nb_streams));

    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
        AVCodecParameters* par = m_formatCtx->streams[i]->codecpar;
        if (par->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = i;
            m_width = par->width;
            m_height = par->height;
            m_videoTimeBase = m_formatCtx->streams[i]->time_base;

            m_videoCodecPar = avcodec_parameters_alloc();
            avcodec_parameters_copy(m_videoCodecPar, par);

            if (m_formatCtx->streams[i]->avg_frame_rate.den > 0) {
                m_fps = av_q2d(m_formatCtx->streams[i]->avg_frame_rate);
            }

            LOG_INFO("Demuxer", QString("Found video stream %1: %2x%3, fps=%4")
                                    .arg(i).arg(m_width).arg(m_height).arg(m_fps));
            break;
        }
    }

    if (m_videoStreamIndex == -1) {
        LOG_ERROR("Demuxer", "No video stream found");
        emit sigError("No video stream found");
        return false;
    }

    LOG_INFO("Demuxer", "open() success, emitting sigStreamsReady");
    emit sigStreamsReady();
    return true;
}

void Demuxer::close() {
    LOG_INFO("Demuxer", "close()");
    if (m_videoCodecPar) {
        avcodec_parameters_free(&m_videoCodecPar);
        m_videoCodecPar = nullptr;
    }
    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
    }
    m_videoStreamIndex = -1;
    m_eof = false;
}

void Demuxer::startThread() {
    LOG_INFO("Demuxer", "startThread()");
    if (isRunning()) return;
    m_running = true;
    start();
}

void Demuxer::stopThread() {
    LOG_INFO("Demuxer", "stopThread()");
    m_running = false;
    if (isRunning()) wait();
}

void Demuxer::run() {
    LOG_INFO("Demuxer", "Demuxer thread started");

    if (!m_formatCtx || m_videoStreamIndex < 0 || !m_packetQueue) {
        LOG_ERROR("Demuxer", "Invalid state or null pointers in run()");
        return;
    }

    AVPacket* packet = av_packet_alloc();
    int packetCount = 0;
    int videoPacketCount = 0;

    while (m_running && !m_eof) {
        // 核心修复 1：如果队列满了，休眠等待，绝不丢包！
        while (m_packetQueue->size() >= 100 && m_running) {
            QThread::msleep(10);
        }

        // 如果在休眠期间线程被要求停止，则退出循环
        if (!m_running) break;

        int ret = av_read_frame(m_formatCtx, packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                m_eof = true;
                LOG_INFO("Demuxer", "End of file (EOF)");
                break;
            }
            continue;
        }

        packetCount++;
        if (packet->stream_index == m_videoStreamIndex) {
            videoPacketCount++;
            if (videoPacketCount % 100 == 0) {
                LOG_DEBUG("Demuxer", QString("Read video packet #%1, queue size: %2")
                              .arg(videoPacketCount).arg(m_packetQueue->size()));
            }

            Packet pkt = Packet::fromAVPacket(packet);
            // 此时队列一定有空间（或正在退出），安全放入
            m_packetQueue->push(std::move(pkt));
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    LOG_INFO("Demuxer", QString("Demuxer thread exited: totalPackets=%1, videoPackets=%2")
                            .arg(packetCount).arg(videoPacketCount));
}