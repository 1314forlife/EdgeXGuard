#include "FaceRepositoryImpl.h"
#include "presentation/viewmodel/face_database/FaceDatabaseViewModel.h"

// 确保返回值、类名作用域、参数类型与头文件完全严丝合缝
bool FaceRepositoryImpl::addFace(const FaceEntity& face) {
    qDebug() << "[FaceRepositoryImpl仓储层] 收到录入人脸请求，姓名:" << face.name << "工号:" << face.workId;
    // 现在的录入流走的是 ViewModel 的内存 QList，底层仓储暂时直接返回真即可。
    // 未来换真实 SQLite 时，这里就会被替换为标准的 SQL 插入语句。
    return true;
}

std::optional<FaceEntity> FaceRepositoryImpl::searchNearestFace(const std::vector<float>& faceFeature, float threshold) {
    (void)faceFeature;
    (void)threshold;

    QString recognizedName = "张三";
    QString denyReason;

    if (FaceDatabaseViewModel::isUserAllowed(recognizedName, denyReason)) {
        FaceEntity entity;
        entity.id = 1;
        entity.name = recognizedName;
        entity.workId = "HQ-9527";
        return entity;
    }
    return std::nullopt;
}