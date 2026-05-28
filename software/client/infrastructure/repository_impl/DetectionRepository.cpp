#include "DetectionRepository.h"
#include <QDateTime>
#include <algorithm>

std::vector<DetectionInfo> DetectionRepository::getLatestDetections(int count)
{
    std::vector<DetectionInfo> result;
    int size = static_cast<int>(m_detections.size());
    int start = std::max(0, size - count);
    for (int i = m_detections.size() - 1; i >= start; --i) {
        result.push_back(m_detections[i]);
    }
    return result;
}

bool DetectionRepository::addDetection(const DetectionInfo& detection)
{
    DetectionInfo newDetection = detection;
    newDetection.id = m_nextId++;

    if (newDetection.timestamp == 0) {
        newDetection.timestamp = QDateTime::currentMSecsSinceEpoch();
    }

    m_detections.append(newDetection);

    while (m_detections.size() > 100) {
        m_detections.removeFirst();
    }

    return true;
}