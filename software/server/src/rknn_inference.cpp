#include "rknn_inference.h"
#include "config.h"
#include "face_database.h"
#include <iostream>
#include <cmath>
#include <algorithm>

// 辅助数学函数：计算两个 512 维特征向量的余弦相似度
static float calculateCosineSimilarity(const float* v1, const std::vector<float>& v2, int dimensions) {
    float dot_product = 0.0f, denom_a = 0.0f, denom_b = 0.0f;
    for (int i = 0; i < dimensions; ++i) {
        dot_product += v1[i] * v2[i];
        denom_a += v1[i] * v1[i];
        denom_b += v2[i] * v2[i];
    }
    if (denom_a == 0.0f || denom_b == 0.0f) return 0.0f;
    return dot_product / (std::sqrt(denom_a) * std::sqrt(denom_b));
}

RKNNInference& RKNNInference::instance() {
    static RKNNInference inst;
    return inst;
}

RKNNInference::RKNNInference() : m_ctx(0), m_worker_running(false), m_face_detected(false) {
    m_face_rect = cv::Rect(0, 0, 0, 0);
    m_display_name = "Unknown";
    m_priv_status = "No_Face";
}

RKNNInference::~RKNNInference() {
    destroy();
}

bool RKNNInference::init(const std::string& model_path) {
    FILE* fp = fopen(model_path.c_str(), "rb");
    if (!fp) {
        std::cerr << "❌ 无法打开模型文件: " << model_path << std::endl;
        return false;
    }
    fseek(fp, 0, SEEK_END);
    int model_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::vector<char> model_data(model_size);
    if (fread(model_data.data(), 1, model_size, fp) != model_size) {
        fclose(fp);
        return false;
    }
    fclose(fp);

    int ret = rknn_init(&m_ctx, model_data.data(), model_size, 0, nullptr);
    if (ret < 0) {
        std::cerr << "❌ rknn_init 失败, 错误码: " << ret << std::endl;
        return false;
    }

    ret = rknn_query(m_ctx, RKNN_QUERY_IN_OUT_NUM, &m_io_num, sizeof(m_io_num));
    if (ret < 0) return false;

    return true;
}

void RKNNInference::destroy() {
    if (m_ctx > 0) {
        rknn_destroy(m_ctx);
        m_ctx = 0;
    }
}

void RKNNInference::submitTask(const InferenceTask& task) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    if (m_task_queue.size() > 2) {
        m_task_queue.pop(); 
    }
    m_task_queue.push(task);
    m_queue_cv.notify_one();
}

void RKNNInference::getLastResult(std::string& name, std::string& status, bool& detected, cv::Rect& rect) {
    std::lock_guard<std::mutex> lock(m_result_mutex);
    name = m_display_name;
    status = m_priv_status;
    detected = m_face_detected;
    rect = m_face_rect;
}

void RKNNInference::startWorker() {
    m_worker_running = true;
    m_worker_thread = std::thread(&RKNNInference::workerLoop, this);
    std::cout << "🚀 NPU 异步推理后台工作线程已成功启动..." << std::endl;
}

void RKNNInference::stopWorker() {
    m_worker_running = false;
    m_queue_cv.notify_all();
    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
    }
    std::cout << "🛑 NPU 异步推理后台工作线程已安全停止。" << std::endl;
}

// 🩺 完全重构底图特征提取提取函数，同步为 25200x85 排布
bool RKNNInference::extractFeatureDirect(const cv::Mat& input_640, std::vector<float>& feature_out) {
    if (m_ctx == 0 || input_640.empty()) {
        return false;
    }
    
    cv::Mat rgb_mat;
    cv::cvtColor(input_640, rgb_mat, cv::COLOR_BGR2RGB);
    
    rknn_input inputs[1];
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].size = rgb_mat.cols * rgb_mat.rows * rgb_mat.channels();
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].buf = rgb_mat.data;
    
    if (rknn_inputs_set(m_ctx, m_io_num.n_input, inputs) < 0) return false;
    if (rknn_run(m_ctx, nullptr) < 0) return false;
    
    std::vector<rknn_output> outputs(m_io_num.n_output);
    memset(outputs.data(), 0, sizeof(rknn_output) * m_io_num.n_output);
    for (uint32_t i = 0; i < m_io_num.n_output; ++i) {
        outputs[i].want_float = 1;
    }
    
    if (rknn_outputs_get(m_ctx, m_io_num.n_output, outputs.data(), nullptr) >= 0) {
        if (m_io_num.n_output >= 1 && outputs[0].buf != nullptr) {
            float* output = (float*)outputs[0].buf;
            int num_boxes = 8400;
            float max_conf = 0.0f;
            for (int i = 0; i < num_boxes; ++i) {
                float raw_conf = output[4 * num_boxes + i];
                float conf = 1.0f / (1.0f + exp(-raw_conf));
                if (conf > max_conf) max_conf = conf;
            }
            feature_out.clear();
            feature_out.push_back(max_conf);
            rknn_outputs_release(m_ctx, m_io_num.n_output, outputs.data());
            return max_conf > 0.3f;
        }
        rknn_outputs_release(m_ctx, m_io_num.n_output, outputs.data());
    }
    return false;
}

void RKNNInference::workerLoop() {
    while (m_worker_running) {
        InferenceTask task;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_queue_cv.wait(lock, [this]() { return !m_task_queue.empty() || !m_worker_running; });
            if (!m_worker_running) break;
            task = m_task_queue.front();
            m_task_queue.pop();
        }

        if (task.roi_mat.empty()) continue;

        cv::Mat rgb_mat;
        cv::cvtColor(task.roi_mat, rgb_mat, cv::COLOR_BGR2RGB);
        
        rknn_input inputs[1];
        memset(inputs, 0, sizeof(inputs));
        inputs[0].index = 0;
        inputs[0].type = RKNN_TENSOR_UINT8;
        inputs[0].size = rgb_mat.cols * rgb_mat.rows * rgb_mat.channels();
        inputs[0].fmt = RKNN_TENSOR_NHWC;
        inputs[0].buf = rgb_mat.data;
        
        if (rknn_inputs_set(m_ctx, m_io_num.n_input, inputs) < 0) continue;
        if (rknn_run(m_ctx, nullptr) < 0) continue;
        
        std::vector<rknn_output> outputs(m_io_num.n_output);
        memset(outputs.data(), 0, sizeof(rknn_output) * m_io_num.n_output);
        for (uint32_t i = 0; i < m_io_num.n_output; ++i) {
            outputs[i].want_float = 1;
        }
        
        if (rknn_outputs_get(m_ctx, m_io_num.n_output, outputs.data(), nullptr) >= 0) {
            if (m_io_num.n_output >= 1 && outputs[0].buf != nullptr) {
                float* output = (float*)outputs[0].buf;
                
                int num_boxes = 8400;
                int num_attrs = 84;
                
                int img_cx = ROI_WIDTH / 2;
                int img_cy = ROI_HEIGHT / 2;
                int best_dist = 9999;
                float best_conf = 0.0f;
                cv::Rect best_rect(0, 0, 0, 0);
                bool face_found = false;
                
                for (int i = 0; i < num_boxes; ++i) {
                    int offset = i * num_attrs;
                    
                    float x1 = output[offset + 0];
                    float y1 = output[offset + 1];
                    float x2 = output[offset + 2];
                    float y2 = output[offset + 3];
                    
                    float raw_conf = output[offset + 4];
                    float conf = 1.0f / (1.0f + exp(-raw_conf));
                    
                    int px = (int)x1;
                    int py = (int)y1;
                    int pw = (int)(x2 - x1);
                    int ph = (int)(y2 - y1);
                    
                    // 边界裁剪
                    if (px < 0) px = 0;
                    if (py < 0) py = 0;
                    if (px + pw > ROI_WIDTH) pw = ROI_WIDTH - px;
                    if (py + ph > ROI_HEIGHT) ph = ROI_HEIGHT - py;
                    
                    // 合理的人脸框：大小适中
                    if (conf > 0.3f && pw > 60 && ph > 60 && pw < 250 && ph < 250) {
                        int box_cx = px + pw/2;
                        int box_cy = py + ph/2;
                        int dist = abs(box_cx - img_cx) + abs(box_cy - img_cy);
                        
                        if (dist < best_dist) {
                            best_dist = dist;
                            best_conf = conf;
                            best_rect = cv::Rect(px, py, pw, ph);
                            face_found = true;
                        }
                    }
                }
                
                {
                    std::lock_guard<std::mutex> lock(m_result_mutex);
                    m_face_detected = face_found;
                    if (face_found) {
                        m_face_rect = best_rect;
                        m_display_name = "Face";
                        m_priv_status = "Conf: " + std::to_string((int)(best_conf * 100)) + "%";
                        
                        static int log_cnt = 0;
                        if (log_cnt++ % 10 == 0) {
                            std::cout << "✅ 人脸检测: conf=" << best_conf
                                      << ", rect=(" << best_rect.x << "," << best_rect.y
                                      << "," << best_rect.width << "," << best_rect.height << ")" << std::endl;
                        }
                    }
                }
            }
            rknn_outputs_release(m_ctx, m_io_num.n_output, outputs.data());
        }
    }
    std::cout << "🧠 NPU 后台异步推理线程已安全退出" << std::endl;
}