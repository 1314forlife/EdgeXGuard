#include "face_database.h"
#include <iostream>
#include <fstream>
#include <sstream>

FaceDatabase& FaceDatabase::instance() {
    static FaceDatabase db;
    return db;
}

void FaceDatabase::registerFace(const std::string& name, const std::string& workId, 
                                 const std::string& imagePath, const cv::Mat& faceRoi) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    RegisteredFace face;
    face.id = m_faces.size() + 1;
    face.name = name;
    face.workId = workId;
    face.privilege = "允许通行";
    face.time = "2026-06-11";
    face.imagePath = imagePath;
    face.label = m_nextLabel++;
    m_faces.push_back(face);
    m_labelToName[face.label] = name;
    
    if (m_recognizer.empty()) {
        m_recognizer = cv::face::LBPHFaceRecognizer::create();
    }
    
    std::vector<cv::Mat> images;
    std::vector<int> labels;
    for (const auto& f : m_faces) {
        cv::Mat img = cv::imread(f.imagePath, cv::IMREAD_GRAYSCALE);
        if (!img.empty()) {
            cv::resize(img, img, cv::Size(100, 100));
            images.push_back(img);
            labels.push_back(f.label);
        }
    }
    
    if (!images.empty()) {
        if (m_faces.size() == 1) {
            m_recognizer->train(images, labels);
        } else {
            m_recognizer->update(images, labels);
        }
    }
    
    std::cout << "✅ 注册人脸: " << name << std::endl;
}

std::string FaceDatabase::recognizeFace(const cv::Mat& faceRoi, double& confidence) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_recognizer.empty() || m_faces.empty()) {
        confidence = 100.0;
        return "Unknown";
    }
    
    cv::Mat gray;
    if (faceRoi.channels() == 3) {
        cv::cvtColor(faceRoi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = faceRoi;
    }
    
    cv::Mat resized;
    cv::resize(gray, resized, cv::Size(100, 100));
    
    // 提高阈值，让识别更宽容
    m_recognizer->setThreshold(120.0);
    
    int label;
    m_recognizer->predict(resized, label, confidence);
    
    auto it = m_labelToName.find(label);
    if (it != m_labelToName.end() && confidence < 120.0) {
        return it->second;
    }
    
    return "Unknown";
}

void FaceDatabase::saveToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    for (const auto& face : m_faces) {
        file << face.id << "," << face.name << "," << face.workId << "," 
             << face.privilege << "," << face.time << "," << face.imagePath << "\n";
    }
    file.close();
}

void FaceDatabase::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        RegisteredFace face;
        std::getline(ss, token, ','); face.id = std::stoi(token);
        std::getline(ss, face.name, ',');
        std::getline(ss, face.workId, ',');
        std::getline(ss, face.privilege, ',');
        std::getline(ss, face.time, ',');
        std::getline(ss, face.imagePath, ',');
        face.label = m_nextLabel++;
        m_faces.push_back(face);
        m_labelToName[face.label] = face.name;
    }
    file.close();
}