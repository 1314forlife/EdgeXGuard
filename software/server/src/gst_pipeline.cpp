#include "gst_pipeline.h"
#include "config.h"
#include <iostream>

MyGstPipeline::MyGstPipeline() {
    m_loop = g_main_loop_new(nullptr, FALSE);
}

MyGstPipeline::~MyGstPipeline() {
    stop();
    if (m_loop) g_main_loop_unref(m_loop);
}

bool MyGstPipeline::createInputPipeline(const std::string& rtsp_url) {
    std::string pipeline_str = 
        "rtspsrc location=" + rtsp_url + 
        " latency=100 protocols=tcp tcp-timeout=5000000 ! "
        "application/x-rtp,media=video,encoding-name=H265 ! "
        "rtph265depay ! h265parse config-interval=1 ! mppvideodec ! "
        "video/x-raw,format=NV12 ! videoconvert ! video/x-raw,format=BGR ! "
        "appsink name=my_appsink emit-signals=true max-buffers=2 drop=true sync=false async=false";
    
    GError* error = nullptr;
    m_input_pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!m_input_pipeline || error) {
        std::cerr << "❌ 创建拉流管线失败: " << (error ? error->message : "未知错误") << std::endl;
        if (error) g_error_free(error);
        return false;
    }
    
    GstElement* appsink = gst_bin_get_by_name(GST_BIN(m_input_pipeline), "my_appsink");
    if (appsink) {
        g_signal_connect(appsink, "new-sample", G_CALLBACK(onNewSample), this);
        gst_object_unref(appsink);
    }
    
    return true;
}

bool MyGstPipeline::createOutputPipeline(const std::string& rtmp_url) {
    std::string pipeline_str = 
        "appsrc name=my_appsrc format=time is-live=true do-timestamp=false ! "
        "video/x-raw,format=BGR,width=" + std::to_string(OUTPUT_WIDTH) + 
        ",height=" + std::to_string(OUTPUT_HEIGHT) + ",framerate=" + std::to_string(OUTPUT_FPS) + "/1 ! "
        "videoconvert ! video/x-raw,format=NV12 ! "
        "mpph264enc rc-mode=vbr gop=30 bps=4000000 ! h264parse config-interval=1 ! "
        "flvmux streamable=true ! rtmpsink location=" + rtmp_url + " sync=false async=false";
    
    GError* error = nullptr;
    m_output_pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!m_output_pipeline || error) {
        std::cerr << "❌ 创建推流管线失败: " << (error ? error->message : "未知错误") << std::endl;
        if (error) g_error_free(error);
        return false;
    }
    
    m_appsrc = gst_bin_get_by_name(GST_BIN(m_output_pipeline), "my_appsrc");
    if (m_appsrc) {
        g_object_set(G_OBJECT(m_appsrc), "block", FALSE, nullptr);
        g_object_set(G_OBJECT(m_appsrc), "leaky-type", 1, nullptr);
        g_object_set(G_OBJECT(m_appsrc), "max-bytes", (guint64)18662400, nullptr);
        g_object_set(G_OBJECT(m_appsrc), "format", GST_FORMAT_TIME, nullptr);
    }
    
    return true;
}

void MyGstPipeline::setNewSampleCallback(std::function<void(GstSample*, gpointer)> callback, gpointer user_data) {
    m_sample_callback = callback;
    m_callback_user_data = user_data;
}

void MyGstPipeline::start() {
    if (m_input_pipeline) {
        GstBus* bus = gst_element_get_bus(m_input_pipeline);
        gst_bus_add_watch(bus, onBusMessage, m_loop);
        gst_object_unref(bus);
        gst_element_set_state(m_input_pipeline, GST_STATE_PLAYING);
    }
    
    if (m_output_pipeline) {
        GstBus* bus = gst_element_get_bus(m_output_pipeline);
        gst_bus_add_watch(bus, onBusMessage, m_loop);
        gst_object_unref(bus);
        gst_element_set_state(m_output_pipeline, GST_STATE_PLAYING);
    }
}

void MyGstPipeline::stop() {
    if (m_input_pipeline) {
        gst_element_set_state(m_input_pipeline, GST_STATE_NULL);
        gst_object_unref(m_input_pipeline);
        m_input_pipeline = nullptr;
    }
    
    if (m_output_pipeline) {
        gst_element_set_state(m_output_pipeline, GST_STATE_NULL);
        gst_object_unref(m_output_pipeline);
        m_output_pipeline = nullptr;
    }
    
    if (m_appsrc) {
        gst_object_unref(m_appsrc);
        m_appsrc = nullptr;
    }
}

void MyGstPipeline::run() {
    if (m_loop) {
        g_main_loop_run(m_loop);
    }
}

void MyGstPipeline::quit() {
    if (m_loop) {
        g_main_loop_quit(m_loop);
    }
}

GstFlowReturn MyGstPipeline::onNewSample(GstElement* sink, gpointer user_data) {
    MyGstPipeline* self = static_cast<MyGstPipeline*>(user_data);
    if (!self) return GST_FLOW_OK;
    
    GstSample* sample = nullptr;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (!sample) return GST_FLOW_OK;
    
    if (self->m_sample_callback) {
        self->m_sample_callback(sample, self->m_callback_user_data);
    }
    
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

gboolean MyGstPipeline::onBusMessage(GstBus* bus, GstMessage* msg, gpointer user_data) {
    GMainLoop* loop = static_cast<GMainLoop*>(user_data);
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* err = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(msg, &err, &debug);
            std::cerr << "❌ GStreamer 错误: " << (err ? err->message : "未知") << std::endl;
            if (err) g_error_free(err);
            if (debug) g_free(debug);
            if (loop) g_main_loop_quit(loop);
            break;
        }
        case GST_MESSAGE_EOS:
            if (loop) g_main_loop_quit(loop);
            break;
        default:
            break;
    }
    return TRUE;
}