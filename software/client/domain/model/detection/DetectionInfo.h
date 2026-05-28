#ifndef DETECTIONINFO_H
#define DETECTIONINFO_H

#include <QString>
#include <QRect>

enum class ObjectClass {
    PERSON,
    CAR,
    BICYCLE,
    MOTORCYCLE,
    BUS,
    TRUCK,
    UNKNOWN
};

struct DetectionInfo
{
    int id = 0;
    ObjectClass objectClass = ObjectClass::UNKNOWN;
    float confidence = 0.0f;
    QRect boundingBox;
    qint64 timestamp = 0;

    QString getClassName() const {
        switch (objectClass) {
        case ObjectClass::PERSON:     return "人";
        case ObjectClass::CAR:        return "汽车";
        case ObjectClass::BICYCLE:    return "自行车";
        case ObjectClass::MOTORCYCLE: return "摩托车";
        case ObjectClass::BUS:        return "公交车";
        case ObjectClass::TRUCK:      return "卡车";
        default:                      return "未知";
        }
    }

    int getConfidencePercent() const {
        return static_cast<int>(confidence * 100);
    }

    bool isReliable() const {
        return confidence > 0.5f;
    }
};

#endif // DETECTIONINFO_H