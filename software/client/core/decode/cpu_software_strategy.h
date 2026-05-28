#ifndef CPU_SOFTWARE_STRATEGY_H
#define CPU_SOFTWARE_STRATEGY_H

#include "decoder_strategy.h"

class CpuSoftwareStrategy : public IDecoderStrategy {
public:
    QString createPipeline(const QString& url, uintptr_t winId) override {
        // Windows 调试用软解管道打桩
        return QString("playbin uri=%1 video-sink=\"qmlglsink\"").arg(url);
    }
    QString name() const override { return "CPU_Software_GStreamer"; }
};

#endif // CPU_SOFTWARE_STRATEGY_H