#ifndef GST_PIPELINE_H
#define GST_PIPELINE_H

#include <gst/gst.h>
#include <string>
#include <functional>

class MyGstPipeline {
public:
    MyGstPipeline();
    ~MyGstPipeline();

    // 创建拉流管线
    bool createInputPipeline(const std::string& rtsp_url);
    // 创建推流管线
    bool createOutputPipeline(const std::string& rtmp_url);

    // 绑定新帧接收回调函数
    void setNewSampleCallback(std::function<void(GstSample*, gpointer)> callback, gpointer user_data);

    // 状态控制
    void start();
    void stop();
    void run();
    void quit();

    // 获取内部推流组件，供外部推送渲染帧
    GstElement* getAppSrc() { return m_appsrc; }

private:
    GMainLoop* m_loop = nullptr;
    GstElement* m_input_pipeline = nullptr;
    GstElement* m_output_pipeline = nullptr;
    GstElement* m_appsrc = nullptr;

    // 保存外部传入的回调逻辑
    std::function<void(GstSample*, gpointer)> m_sample_callback;
    gpointer m_callback_user_data = nullptr;

    // GStreamer 原生内部静态回调
    static GstFlowReturn onNewSample(GstElement* sink, gpointer user_data);
    static gboolean onBusMessage(GstBus* bus, GstMessage* msg, gpointer user_data);
};

#endif // GST_PIPELINE_H