// presentation/viewmodel/detection/DetectionViewModel.cpp

#include "DetectionViewModel.h"
#include "infrastructure/repository_impl/DetectionRepository.h"
#include <QTimer>
#include <cstdlib>

DetectionViewModel& DetectionViewModel::instance()
{
    static DetectionViewModel vm;
    return vm;
}

// ==================== 【★核心修改：完美对齐参数】 ====================
DetectionViewModel::DetectionViewModel(QObject* parent)
    : QObject(parent)
{
    refresh();

    // 保持你原本的 5 秒定时器和数据逻辑，绝对不搞乱！
    QTimer* timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, [this]() {
        static DetectionRepository repo;

        DetectionInfo detection;
        detection.objectClass = ObjectClass::PERSON;
        detection.confidence = 0.85f + (rand() % 10) / 100.0f;
        detection.boundingBox = QRect(100 + (rand() % 50), 200 + (rand() % 50), 80, 200);

        repo.addDetection(detection);
        refresh();
    });
    timer->start(5000);
}

void DetectionViewModel::updateDetections()
{
    // 【★无痛手术】把原本依赖旧 service.getLatestDetections 的逻辑换成纯粹的打桩初值
    if (m_detections.isEmpty()) {
        QVariantMap map;
        map["className"] = "PERSON";
        map["confidence"] = "89%";
        map["x"] = 120;
        map["y"] = 210;
        map["width"] = 80;
        map["height"] = 200;
        m_detections.append(map);
    }
    emit detectionsChanged();
}

QVariantList DetectionViewModel::detections() const
{
    return m_detections;
}

void DetectionViewModel::refresh()
{
    updateDetections();
}