#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// 网络流配置
// ==========================================
// 输入视频源（网络摄像头 RTSP 地址）
#define RTSP_URL "rtsp://127.0.0.1:8554/cam1"
// 输出推流源（本地或远程 RTMP 服务）
#define RTMP_URL "rtmp://127.0.0.1:1935/live/camera1"

// ==========================================
// 路径配置
// ==========================================
// RKNN 模型文件的绝对路径或相对路径
#define MODEL_PATH "/home/radxa/work/EdgeXGuardServer/models/face_detector.rknn"
// 结构化本地数据库的路径
#define DATABASE_PATH "/home/radxa/work/EdgeXGuardServer/storage/data/face_database.json"

// ==========================================
// 推理核心配置 (必须严格匹配你的 RKNN 模型输入维度)
// ==========================================
#define ROI_WIDTH 640
#define ROI_HEIGHT 640
// 抽帧策略：每 3 帧将一帧送入 NPU 队列，大幅缓解嵌入式芯片压力
#define INFERENCE_INTERVAL 3  
// 过滤杂讯的置信度阈值
#define CONFIDENCE_THRESHOLD 0.3f

// ==========================================
// 输出编码与推流配置
// ==========================================
#define OUTPUT_WIDTH 1920
#define OUTPUT_HEIGHT 1080
#define OUTPUT_FPS 30

#endif // CONFIG_H