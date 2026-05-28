#ifndef RKMPP_HARDWARE_STRATEGY_H
#define RKMPP_HARDWARE_STRATEGY_H

#include "decoder_strategy.h"

class RkmppHardwareStrategy : public IDecoderStrategy {
public:
    QString createPipeline(const QString& url, uintptr_t winId) override {
        // RK3588 实机硬解管道打桩
        return QString("rtspsrc location=%1 ! rkmppdec ! glimagesink").arg(url);
    }
    QString name() const override { return "RK3588_MPP_Hardware"; }
};

#endif // RKMPP_HARDWARE_STRATEGY_H