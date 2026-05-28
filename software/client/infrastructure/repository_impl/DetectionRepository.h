#ifndef DETECTIONREPOSITORY_H
#define DETECTIONREPOSITORY_H

#include "domain/repository/IDetectionRepository.h"
#include <QList>

class DetectionRepository : public IDetectionRepository
{
public:
    std::vector<DetectionInfo> getLatestDetections(int count) override;
    bool addDetection(const DetectionInfo& detection) override;

private:
    QList<DetectionInfo> m_detections;
    int m_nextId = 1;
};

#endif // DETECTIONREPOSITORY_H