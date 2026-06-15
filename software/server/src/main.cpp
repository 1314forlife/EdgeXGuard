#include "config.h"
#include "face_database.h"
#include "gst_pipeline.h"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <algorithm>
#include <vector>
#include <cstring>

static int g_frame_count = 0;

std::string getFilenameWithoutExt(const std::string& filepath) {
    size_t last_slash = filepath.find_last_of("/\\");
    std::string filename = (last_slash == std::string::npos) ? filepath : filepath.substr(last_slash + 1);
    size_t last_dot = filename.find_last_of(".");
    return (last_dot == std::string::npos) ? filename : filename.substr(0, last_dot);
}

void onFrameReceived(GstSample* sample, gpointer user_data) {
    static int receive_count = 0;
    receive_count++;
    if (receive_count % 10 == 0) {
        std::cout << "🌊 [流媒体监控] 成功从摄像头抓取并处理了 " << receive_count << " 帧视频！" << std::endl;
    }
    
    MyGstPipeline* pipeline = static_cast<MyGstPipeline*>(user_data);
    GstElement* appsrc = pipeline->getAppSrc();
    if (!appsrc) return;
    
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstCaps* caps = gst_sample_get_caps(sample);
    GstStructure* structure = gst_caps_get_structure(caps, 0);
    
    int width = 0, height = 0;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);
    
    GstClockTime pts = GST_BUFFER_PTS(buffer);
    
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) return;
    
    // BGR 格式直接读取
    cv::Mat frame(height, width, CV_8UC3, (char*)map.data);
    cv::Mat cloned_frame = frame.clone();
    gst_buffer_unmap(buffer, &map);
    
    // Haar 人脸检测
    static cv::CascadeClassifier face_cascade;
    static bool loaded = false;
    if (!loaded) {
        loaded = face_cascade.load("/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml");
        std::cout << "Haar cascade loaded: " << loaded << std::endl;
    }
    
    if (loaded) {
        cv::Mat gray;
        cv::cvtColor(cloned_frame, gray, cv::COLOR_BGR2GRAY);
        std::vector<cv::Rect> faces;
        face_cascade.detectMultiScale(gray, faces, 1.1, 6, 0, cv::Size(80, 80));
        
        for (const auto& face : faces) {
            // 过滤不合理的人脸
            float ratio = (float)face.width / face.height;
            if (ratio < 0.6 || ratio > 1.4) continue;
            if (face.width < 60 || face.height < 60) continue;
            
            // 裁剪人脸 ROI
            cv::Mat faceRoi = cloned_frame(face);
            
            // 识别
            double confidence;
            std::string name = FaceDatabase::instance().recognizeFace(faceRoi, confidence);
            
            // 颜色：绿色 = 已注册，红色 = 陌生人
            cv::Scalar color;
            if (name == "Unknown") {
                color = cv::Scalar(0, 0, 255);  // 红色
            } else {
                color = cv::Scalar(0, 255, 0);  // 绿色
            }
            
            cv::rectangle(cloned_frame, face, color, 3);
            std::string text = name + " (" + std::to_string((int)confidence) + "%)";
            cv::putText(cloned_frame, text, cv::Point(face.x, face.y - 5),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        }
    }
    
    // 推流
    cv::Mat out_frame;
    if (width != OUTPUT_WIDTH || height != OUTPUT_HEIGHT) {
        cv::resize(cloned_frame, out_frame, cv::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT));
    } else {
        out_frame = cloned_frame;
    }
    
    if (!out_frame.isContinuous()) out_frame = out_frame.clone();
    
    size_t buffer_size = out_frame.cols * out_frame.rows * out_frame.channels();
    GstBuffer* out_buffer = gst_buffer_new_allocate(nullptr, buffer_size, nullptr);
    
    GstMapInfo out_map;
    if (gst_buffer_map(out_buffer, &out_map, GST_MAP_WRITE)) {
        memcpy(out_map.data, out_frame.data, buffer_size);
        gst_buffer_unmap(out_buffer, &out_map);
    }
    
    GST_BUFFER_PTS(out_buffer) = pts;
    
    GstFlowReturn ret;
    g_signal_emit_by_name(appsrc, "push-buffer", out_buffer, &ret);
    gst_buffer_unref(out_buffer);
}

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);
    std::cout << "🚀 EdgeXGuard 智能流媒体网关启动" << std::endl;
    
    // 加载本地底图，用 Haar 检测人脸并注册到 LBPH 数据库
    std::vector<std::string> local_images;
    std::vector<std::string> jpg_images;
    std::vector<std::string> png_images;
    
    cv::glob("faces/*.jpg", jpg_images);
    cv::glob("faces/*.png", png_images);
    
    local_images.insert(local_images.end(), jpg_images.begin(), jpg_images.end());
    local_images.insert(local_images.end(), png_images.begin(), png_images.end());
    
    if (!local_images.empty()) {
        std::cout << "📸 [自动注册] 发现 " << local_images.size() << " 张本地底图，正在注册..." << std::endl;
        
        cv::CascadeClassifier reg_cascade;
        reg_cascade.load("/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml");
        
        for (const auto& img_path : local_images) {
            std::string reg_name = getFilenameWithoutExt(img_path);
            
            cv::Mat img = cv::imread(img_path, cv::IMREAD_COLOR);
            if (img.empty()) {
                std::cerr << "❌ 无法读取图片: " << img_path << std::endl;
                continue;
            }
            
            // 检测人脸
            cv::Mat gray;
            cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
            std::vector<cv::Rect> faces;
            reg_cascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(60, 60));
            
            if (!faces.empty()) {
                cv::Mat faceRoi = img(faces[0]);
                FaceDatabase::instance().registerFace(reg_name, reg_name, img_path, faceRoi);
                std::cout << "✅ [注册成功] [" << reg_name << "] 已注入人脸库！" << std::endl;
            } else {
                std::cerr << "❌ [注册失败] 未检测到人脸: " << img_path << std::endl;
            }
        }
    } else {
        std::cout << "⚠️ [警告] faces/ 目录下未检测到任何图片，比对库为空！" << std::endl;
    }
    
    MyGstPipeline pipeline;
    pipeline.setNewSampleCallback([](GstSample* sample, gpointer user_data) {
        onFrameReceived(sample, user_data);
    }, &pipeline);
    
    if (!pipeline.createInputPipeline(RTSP_URL)) return -1;
    if (!pipeline.createOutputPipeline(RTMP_URL)) return -1;
    
    pipeline.start();
    std::cout << "✅ 系统就绪，人脸识别引擎已上线！" << std::endl;
    
    pipeline.run();
    
    pipeline.stop();
    
    std::cout << "👋 程序正常退出" << std::endl;
    return 0;
}