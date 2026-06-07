#ifndef FACE_REPOSITORY_IMPL_H
#define FACE_REPOSITORY_IMPL_H

#include "domain/repository/IFaceRepository.h"

class FaceRepositoryImpl : public IFaceRepository {
public:
    FaceRepositoryImpl(); // 🟢 确保删掉了原先的 = default; 避免重复定义
    virtual ~FaceRepositoryImpl() = default;

    bool addFace(const FaceEntity& face) override;
    std::optional<FaceEntity> searchNearestFace(const std::vector<float>& faceFeature, float threshold) override;

private:
    void initializeDatabase();
};

#endif // FACE_REPOSITORY_IMPL_H