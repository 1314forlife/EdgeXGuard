#ifndef RTP_STREAMER_H
#define RTP_STREAMER_H

#include <string>
#include <gst/gst.h>

/**
 * RTP 推流配置
 */
struct RtpStreamConfig {
    std::string rtsp_url;      // RTSP 源地址
    std::string target_ip;     // 目标 IP
    int target_port;           // 目标端口
    int latency_ms;            // 延迟（毫秒）
    bool sync;                 // 是否同步
    
    RtpStreamConfig() 
        : target_port(5000), latency_ms(0), sync(false) {}
};

/**
 * RTP 推流模块
 * 负责使用 GStreamer 推送 RTP 流
 */
class RtpStreamer {
public:
    RtpStreamer();
    ~RtpStreamer();

    /**
     * 初始化（全局一次）
     */
    static void initGStreamer();
    
    /**
     * 启动推流
     * @param config 推流配置
     * @return 成功返回 true
     */
    bool startStream(const RtpStreamConfig& config);
    
    /**
     * 停止推流
     */
    void stopStream();
    
    /**
     * 是否正在推流
     */
    bool isStreaming() const { return m_pipeline != nullptr; }

private:
    GstElement* m_pipeline;
    GstBus* m_bus;
    guint m_bus_watch_id;
    
    static bool gst_initialized;
    
    // 修复：返回类型改为 gboolean
    static gboolean onBusMessage(GstBus* bus, GstMessage* msg, gpointer user_data);
    
    std::string buildPipeline(const RtpStreamConfig& config);
};

#endif