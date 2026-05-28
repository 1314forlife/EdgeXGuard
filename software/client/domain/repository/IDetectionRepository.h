// client/domain/repository/IDetectionRepository.h
#ifndef IDETECTIONREPOSITORY_H
#define IDETECTIONREPOSITORY_H

#include "../model/detection/DetectionInfo.h"
#include <vector>

class IDetectionRepository
{
public:
    virtual ~IDetectionRepository() = default;

    virtual std::vector<DetectionInfo> getLatestDetections(int count = 10) = 0;
    virtual bool addDetection(const DetectionInfo& detection) = 0;
};

#endif // IDETECTIONREPOSITORY_H