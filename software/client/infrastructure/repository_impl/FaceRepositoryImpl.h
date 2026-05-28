#ifndef FACE_REPOSITORY_IMPL_H
#define FACE_REPOSITORY_IMPL_H

#include "domain/repository/IFaceRepository.h"

class FaceRepositoryImpl : public IFaceRepository {
public:
    FaceRepositoryImpl() = default;
    virtual ~FaceRepositoryImpl() = default; // 确保析构函数有明确实现或 = default

    // 确保这里的签名与接口完全一致
    bool addFace(const FaceEntity& face) override;
    std::optional<FaceEntity> searchNearestFace(const std::vector<float>& faceFeature, float threshold) override;
};

#endif // FACE_REPOSITORY_IMPL_H