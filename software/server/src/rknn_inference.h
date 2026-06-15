#ifndef RKNN_INFERENCE_H
#define RKNN_INFERENCE_H

#include <rknn_api.h>
#include <opencv2/opencv.hpp>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <string>
#include <vector>

struct InferenceTask {
    cv::Mat roi_mat;
    unsigned long long pts;
};

class RKNNInference {
public:
    static RKNNInference& instance();
    bool init(const std::string& model_path);
    void destroy();
    
    void submitTask(const InferenceTask& task);
    void getLastResult(std::string& name, std::string& status, bool& detected, cv::Rect& rect);
    
    // 新增公有直接特征提取接口
    bool extractFeatureDirect(const cv::Mat& input_640, std::vector<float>& feature_out);
    
    void startWorker();
    void stopWorker();

private:
    RKNNInference();
    ~RKNNInference();
    void workerLoop();

    rknn_context m_ctx;
    rknn_input_output_num m_io_num;
    
    std::queue<InferenceTask> m_task_queue;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_cv;
    
    std::thread m_worker_thread;
    bool m_worker_running;
    
    // 缓存结果数据
    std::mutex m_result_mutex;
    std::string m_display_name;
    std::string m_priv_status;
    bool m_face_detected;
    cv::Rect m_face_rect;
};

#endif // RKNN_INFERENCE_H