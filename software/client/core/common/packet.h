#ifndef PACKET_H
#define PACKET_H

#include <memory>
#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
}

class Packet {
public:
    Packet() = default;

    static Packet fromAVPacket(AVPacket* src_pkt) {
        Packet packet;
        if (!src_pkt) return packet;

        // 1. 分配一个新的 AVPacket 壳子
        AVPacket* newPkt = av_packet_alloc();
        if (!newPkt) return packet;

        // 2. 增加底层引用计数，确保多线程安全
        if (av_packet_ref(newPkt, src_pkt) < 0) {
            av_packet_free(&newPkt);
            return packet;
        }

        // 3. 交给智能指针管理。
        // 自定义删除器：先通过 unref 减少引用计数，再安全 free 掉这个壳子
        packet.m_data = std::shared_ptr<AVPacket>(newPkt, [](AVPacket* p) {
            if (p) {
                av_packet_unref(p);  // 明确释放底层数据缓冲区引用
                av_packet_free(&p);  // 彻底释放 AVPacket 结构体本身
            }
        });

        // 4. 同步元数据
        packet.m_streamIndex = src_pkt->stream_index;
        packet.m_pts = src_pkt->pts;
        packet.m_dts = src_pkt->dts;
        packet.m_duration = src_pkt->duration;
        packet.m_size = src_pkt->size;

        return packet;
    }

    AVPacket* get() const { return m_data.get(); }
    int streamIndex() const { return m_streamIndex; }
    int64_t pts() const { return m_pts; }
    int64_t dts() const { return m_dts; }
    int64_t duration() const { return m_duration; }
    int size() const { return m_size; }
    bool isValid() const { return m_data != nullptr && m_size > 0; }

private:
    std::shared_ptr<AVPacket> m_data;
    int m_streamIndex = -1;
    int64_t m_pts = AV_NOPTS_VALUE;
    int64_t m_dts = AV_NOPTS_VALUE;
    int64_t m_duration = 0;
    int m_size = 0;
};

#endif