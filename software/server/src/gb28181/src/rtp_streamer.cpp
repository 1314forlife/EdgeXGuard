#include "rtp_streamer.h"
#include <stdio.h>

bool RtpStreamer::gst_initialized = false;

RtpStreamer::RtpStreamer() : m_pipeline(nullptr), m_bus(nullptr), m_bus_watch_id(0) {
    initGStreamer();
}

RtpStreamer::~RtpStreamer() {
    stopStream();
}

void RtpStreamer::initGStreamer() {
    if (!gst_initialized) {
        gst_init(NULL, NULL);
        gst_initialized = true;
        printf("✅ GStreamer 初始化成功\n");
    }
}

bool RtpStreamer::startStream(const RtpStreamConfig& config) {
    stopStream();
    
    printf("🎬 [RTP推流] 目标: %s:%d\n", 
           config.target_ip.c_str(), config.target_port);
    
    std::string pipeline_desc = buildPipeline(config);
    printf("🎬 推流管道:\n%s\n", pipeline_desc.c_str());
    
    GError* error = NULL;
    m_pipeline = gst_parse_launch(pipeline_desc.c_str(), &error);
    
    if (error) {
        printf("❌ 构建推流管道失败: %s\n", error->message);
        g_error_free(error);
        return false;
    }
    
    // 设置总线监听 - 修复回调函数类型
    m_bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
    m_bus_watch_id = gst_bus_add_watch(m_bus, (GstBusFunc)onBusMessage, this);
    
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    printf("✅ RTP 推流已启动\n");
    return true;
}

void RtpStreamer::stopStream() {
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        
        if (m_bus_watch_id > 0) {
            g_source_remove(m_bus_watch_id);
            m_bus_watch_id = 0;
        }
        
        if (m_bus) {
            gst_object_unref(m_bus);
            m_bus = nullptr;
        }
        
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
        printf("✅ RTP 推流已停止\n");
    }
}

std::string RtpStreamer::buildPipeline(const RtpStreamConfig& config) {
    char pipeline[1024];
    snprintf(pipeline, sizeof(pipeline),
        "rtspsrc location=%s latency=%d ! "
        "rtph264depay ! "
        "h264parse ! "
        "rtph264pay config-interval=1 ! "
        "udpsink host=%s port=%d sync=%s",
        config.rtsp_url.c_str(),
        config.latency_ms,
        config.target_ip.c_str(),
        config.target_port,
        config.sync ? "true" : "false"
    );
    return std::string(pipeline);
}

// 修复：返回 gboolean 而不是 void
gboolean RtpStreamer::onBusMessage(GstBus* bus, GstMessage* msg, gpointer user_data) {
    (void)bus;  // 消除未使用参数警告
    
    RtpStreamer* self = static_cast<RtpStreamer*>(user_data);
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* err = NULL;
            gchar* debug = NULL;
            gst_message_parse_error(msg, &err, &debug);
            printf("❌ GStreamer 错误: %s\n", err->message);
            g_error_free(err);
            g_free(debug);
            self->stopStream();
            break;
        }
        case GST_MESSAGE_EOS:
            printf("📹 GStreamer EOS\n");
            break;
        default:
            break;
    }
    
    return TRUE;  // 保持回调继续
}