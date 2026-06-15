#ifndef FACE_DATABASE_H
#define FACE_DATABASE_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>

struct RegisteredFace {
    int id;
    std::string name;
    std::string workId;
    std::string privilege;
    std::string time;
    std::string imagePath;
    int label;
};

class FaceDatabase {
public:
    static FaceDatabase& instance();
    
    // 注册人脸（用 OpenCV Mat）
    void registerFace(const std::string& name, const std::string& workId, 
                      const std::string& imagePath, const cv::Mat& faceRoi);
    
    // 识别人脸
    std::string recognizeFace(const cv::Mat& faceRoi, double& confidence);
    
    // 获取所有注册人脸
    const std::vector<RegisteredFace>& getRegisteredFaces() const { return m_faces; }
    
    // 保存/加载
    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);
    
    size_t size() const { return m_faces.size(); }
    
private:
    FaceDatabase() = default;
    std::vector<RegisteredFace> m_faces;
    std::map<int, std::string> m_labelToName;
    cv::Ptr<cv::face::LBPHFaceRecognizer> m_recognizer;
    std::mutex m_mutex;
    int m_nextLabel = 0;
};

#endif // FACE_DATABASE_H