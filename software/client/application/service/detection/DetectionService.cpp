#include "DetectionService.h"
#include <QDebug>

DetectionService::DetectionService(
    std::shared_ptr<IFaceRepository> faceRepo,
    std::shared_ptr<IDoorAccessRepository> doorRepo,
    std::shared_ptr<IAlarmRepository> alarmRepo,
    QObject *parent)
    : QObject(parent)
    , m_faceRepo(std::move(faceRepo))
    , m_doorRepo(std::move(doorRepo))
    , m_alarmRepo(std::move(alarmRepo))
{
}

void DetectionService::processFaceVerification(const std::vector<float>& faceFeature) {
    qDebug() << "[DetectionService] 开始人脸比对流水线...";

    // 默认走我们的打桩仓储
    auto matchedFace = m_faceRepo->searchNearestFace(faceFeature, 0.85f);

    if (matchedFace.has_value()) {
        qDebug() << "[DetectionService] 验证通过:" << matchedFace->name;
        m_doorRepo->openDoor();
        emit doorOpened(matchedFace->name);
    } else {
        qDebug() << "[DetectionService] 未登记的陌生人！";
        m_alarmRepo->triggerAlarm(5000);
        emit strangerAlertTriggered();
    }
}