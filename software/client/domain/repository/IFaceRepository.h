#ifndef IFACE_REPOSITORY_H
#define IFACE_REPOSITORY_H

#include "domain/model/detection/FaceEntity.h"
#include <optional>

class IFaceRepository {
public:
    virtual ~IFaceRepository() = default;

    // 录入人脸
    virtual bool addFace(const FaceEntity& face) = 0;
    // 刷脸比对（输入当前人脸特征，返回最相似的用户）
    virtual std::optional<FaceEntity> searchNearestFace(const std::vector<float>& targetFeature, float threshold) = 0;
};

#endif // IFACE_REPOSITORY_H