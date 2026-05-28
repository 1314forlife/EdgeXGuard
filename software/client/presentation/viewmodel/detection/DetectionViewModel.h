#ifndef DETECTION_VIEWMODEL_H
#define DETECTION_VIEWMODEL_H

#include <QObject>
#include <QVariantList>

class DetectionViewModel : public QObject {
    Q_OBJECT
public:
    static DetectionViewModel& instance();

    // 恢复你原本的接口名称
    QVariantList detections() const;

public slots:
    void updateDetections();
    void refresh();

signals:
    // 恢复你原本的信号名称
    void detectionsChanged();

private:
    // 构造函数带上 parent 参数，符合 Qt 规范
    explicit DetectionViewModel(QObject* parent = nullptr);
    ~DetectionViewModel() = default;

    DetectionViewModel(const DetectionViewModel&) = delete;
    DetectionViewModel& operator=(const DetectionViewModel&) = delete;

    QVariantList m_detections;
};

#endif // DETECTION_VIEWMODEL_H