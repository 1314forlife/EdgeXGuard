#ifndef DECODER_STRATEGY_H
#define DECODER_STRATEGY_H

#include <QString>
#include <cstdint>

class IDecoderStrategy {
public:
    virtual ~IDecoderStrategy() = default;
    virtual QString createPipeline(const QString& url, uintptr_t winId) = 0;
    virtual QString name() const = 0;
};

#endif // DECODER_STRATEGY_H