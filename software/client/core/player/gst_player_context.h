#ifndef GST_PLAYER_CONTEXT_H
#define GST_PLAYER_CONTEXT_H

#include <memory>
#include <QString>
#include "core/decode/decoder_strategy.h"

class GstPlayerContext {
public:
    GstPlayerContext();
    ~GstPlayerContext();

    void setStrategy(std::unique_ptr<IDecoderStrategy> strategy);
    void play(const QString& url, uintptr_t winId);
    void stop();

private:
    std::unique_ptr<IDecoderStrategy> m_strategy;
};

#endif // GST_PLAYER_CONTEXT_H