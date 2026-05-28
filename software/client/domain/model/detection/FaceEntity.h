#ifndef FACE_ENTITY_H
#define FACE_ENTITY_H

#include <QString>
#include <vector>

struct FaceEntity {
    int id = 0;
    QString name;
    QString workId;
    std::vector<float> feature; // 512维人脸特征向量
};

#endif // FACE_ENTITY_H