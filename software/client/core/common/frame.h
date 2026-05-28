#ifndef FRAME_H
#define FRAME_H

#include <memory>
#include <cstdint>
#include "core/common/logger.h" // 如果需要打日志的话

extern "C" {
#include <libavutil/frame.h>
}

class FrameData {
public:
    FrameData() = default;

    static FrameData fromAVFrame(AVFrame* src_frame, int64_t pts) {
        FrameData data;

        if (!src_frame || !src_frame->data[0]) {
            return data;
        }

        // 1. 分配一个全新的 AVFrame 结构体
        AVFrame* dst_frame = av_frame_alloc();
        if (!dst_frame) {
            return data;
        }

        // 2. 核心魔法：使用 av_frame_ref 增加底层数据的引用计数！
        // 这样即使 src_frame 被 unref，底层的像素内存也不会被释放
        if (av_frame_ref(dst_frame, src_frame) < 0) {
            av_frame_free(&dst_frame);
            return data;
        }

        // 3. 把这个全新的 dst_frame 交给 shared_ptr 管理
        data.m_frame = std::shared_ptr<AVFrame>(dst_frame, [](AVFrame* f) {
            if (f) {
                // 当 shared_ptr 销毁时，先解除数据引用，再释放结构体
                av_frame_unref(f);
                av_frame_free(&f);
            }
        });

        data.m_pts = pts;
        data.m_width = dst_frame->width;
        data.m_height = dst_frame->height;

        for (int i = 0; i < 4; i++) {
            data.m_linesize[i] = dst_frame->linesize[i];
        }

        return data;
    }

    uint8_t* getY() const { return m_frame ? m_frame->data[0] : nullptr; }
    uint8_t* getU() const { return m_frame ? m_frame->data[1] : nullptr; }
    uint8_t* getV() const { return m_frame ? m_frame->data[2] : nullptr; }

    const int* linesize() const { return m_linesize; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    int64_t pts() const { return m_pts; }

    // 增加数据有效性校验
    bool isValid() const {
        return m_frame != nullptr && m_width > 0 && m_height > 0 && getY() != nullptr;
    }

private:
    std::shared_ptr<AVFrame> m_frame;
    int m_linesize[4] = {0};
    int m_width = 0;
    int m_height = 0;
    int64_t m_pts = 0;
};

#endif