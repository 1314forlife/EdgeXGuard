#ifndef DETECTION_SERVICE_H
#define DETECTION_SERVICE_H

#include <QObject>
#include <memory>
#include <vector>
#include "domain/repository/IFaceRepository.h"
#include "domain/repository/IDoorAccessRepository.h"
#include "domain/repository/IAlarmRepository.h"

class DetectionService : public QObject {
    Q_OBJECT
public:
    // ✅ 确保这里的构造函数声明接收 4 个参数
    explicit DetectionService(
        std::shared_ptr<IFaceRepository> faceRepo,
        std::shared_ptr<IDoorAccessRepository> doorRepo,
        std::shared_ptr<IAlarmRepository> alarmRepo,
        QObject *parent = nullptr
        );

    void processFaceVerification(const std::vector<float>& faceFeature);

signals:
    void doorOpened(const QString& userName);
    void strangerAlertTriggered();

private:
    std::shared_ptr<IFaceRepository> m_faceRepo;
    std::shared_ptr<IDoorAccessRepository> m_doorRepo;
    std::shared_ptr<IAlarmRepository> m_alarmRepo;
};

#endif // DETECTION_SERVICE_H