#ifndef FACECAPTUREDIALOG_H
#define FACECAPTUREDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

// 🟢 引入 Qt6 多媒体核心组件
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

class FaceCaptureDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FaceCaptureDialog(QWidget *parent = nullptr);
    ~FaceCaptureDialog(); // 👈 改为手动析构，用来安全关闭摄像头
    QString getName() const { return m_nameEdit->text().trimmed(); }
    QString getWorkId() const { return m_workIdEdit->text().trimmed(); }

private slots:
    void onSaveButtonClicked();
    void handleVideoFrame(const QVideoFrame &frame); // 👈 核心槽函数：接收摄像头原始帧

private:
    // 布局与控件
    QVBoxLayout *m_mainLayout;
    QLabel *m_videoLabel;
    QLineEdit *m_nameEdit;
    QLineEdit *m_workIdEdit;
    QPushButton *m_saveButton;
    QPushButton *m_cancelButton;

    cv::Mat m_bestFaceROI;
    cv::Rect m_bestFaceRect;
    QString m_bestFaceQualityReason;

    // 🟢 摄像头控制三件套
    QCamera* m_camera;
    QMediaCaptureSession* m_captureSession;
    QVideoSink* m_videoSink;

    // 📸 用来暂存当前最新一帧，抓拍时直接用它
    QVideoFrame            m_currentFrame;
};

#endif // FACECAPTUREDIALOG_H