#include "facecapturedialog.h"
#include "presentation/viewmodel/face_database/FaceDatabaseViewModel.h"
#include <QMessageBox>
#include <QDebug>
#include <QMediaDevices>
#include <QImage>
#include <QPixmap>
#include <QDateTime>
#include <QDir>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

// ============================================================================
// 🛡️ 人脸质量校验辅助函数
// ============================================================================

// 1. 清晰度检测（拉普拉斯方差法）
static double getImageBlurriness(const cv::Mat& gray) {
    cv::Mat laplacian;
    cv::Laplacian(gray, laplacian, CV_64F);
    cv::Scalar mean, stddev;
    cv::meanStdDev(laplacian, mean, stddev);
    return stddev[0] * stddev[0];
}

// 2. 光照均匀度检测
static bool isLightingGood(const cv::Mat& gray) {
    cv::Scalar mean, stddev;
    cv::meanStdDev(gray, mean, stddev);
    double avg = mean[0];
    // 光照正常范围：60-200（0-255范围）
    return (avg > 60 && avg < 200);
}

// 3. 人脸占比检测
static bool isFaceSizeValid(const cv::Rect& face, int frameWidth, int frameHeight) {
    int faceArea = face.width * face.height;
    int frameArea = frameWidth * frameHeight;
    float faceRatio = (float)faceArea / frameArea;
    // 人脸占比应该大于 5%，小于 60%
    return (faceRatio > 0.05 && faceRatio < 0.6);
}

// 4. 综合质量校验
static bool isFaceQualityValid(const cv::Mat& faceROI, const cv::Rect& face, 
                                int frameWidth, int frameHeight, 
                                std::string& outReason) {
    if (faceROI.empty()) {
        outReason = "人脸区域为空";
        return false;
    }
    
    // 尺寸校验
    if (face.width < 80 || face.height < 80) {
        outReason = "人脸太小，请靠近摄像头";
        return false;
    }
    
    if (face.width > 500 || face.height > 500) {
        outReason = "人脸太大，请远离摄像头";
        return false;
    }
    
    // 人脸占比校验
    if (!isFaceSizeValid(face, frameWidth, frameHeight)) {
        outReason = "人脸占比异常，请调整距离";
        return false;
    }
    
    // 灰度化用于质量检测
    cv::Mat gray;
    if (faceROI.channels() == 3) {
        cv::cvtColor(faceROI, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = faceROI;
    }
    
    // 光照检测
    if (!isLightingGood(gray)) {
        outReason = "光线过暗或过亮，请调整环境光";
        return false;
    }
    
    // 清晰度检测
    double blurScore = getImageBlurriness(gray);
    if (blurScore < 50.0) {
        outReason = "图像模糊，请保持稳定";
        return false;
    }
    
    outReason = "合格";
    return true;
}

// ============================================================================
// FaceCaptureDialog 实现
// ============================================================================

FaceCaptureDialog::FaceCaptureDialog(QWidget *parent) : QDialog(parent),
    m_camera(nullptr), m_captureSession(nullptr), m_videoSink(nullptr)
{
    this->setWindowTitle("实时人脸凭证采集仓");
    this->setFixedSize(500, 550);
    this->setStyleSheet("background-color: #0f172a; color: #e2e8f0;");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    m_mainLayout->setSpacing(15);

    m_videoLabel = new QLabel(this);
    m_videoLabel->setFixedSize(460, 320);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setText("📷 正在唤醒本地硬件摄像头驱动...");
    m_videoLabel->setStyleSheet("background-color: #020617; border: 2px dashed #334155; border-radius: 8px; color: #94a3b8; font-size: 14px;");
    m_mainLayout->addWidget(m_videoLabel);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignRight);

    m_nameEdit = new QLineEdit(this);
    m_workIdEdit = new QLineEdit(this);
    QString editStyle = "QLineEdit { background-color: #1e293b; border: 1px solid #475569; border-radius: 6px; padding: 6px; color: white; min-width: 200px; }"
                        "QLineEdit:focus { border: 1px solid #3b82f6; }";
    m_nameEdit->setStyleSheet(editStyle);
    m_workIdEdit->setStyleSheet(editStyle);

    formLayout->addRow(new QLabel("👤 用户姓名：", this), m_nameEdit);
    formLayout->addRow(new QLabel("🆔 工号/卡号：", this), m_workIdEdit);
    m_mainLayout->addLayout(formLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("🎯 抓拍并保存", this);
    m_cancelButton = new QPushButton("取消", this);
    m_saveButton->setStyleSheet("background-color: #2563eb; color: white; padding: 10px 20px; border-radius: 6px; font-weight: bold; font-size: 13px;");
    m_cancelButton->setStyleSheet("background-color: #334155; color: #cbd5e1; padding: 10px 20px; border-radius: 6px; font-size: 13px;");
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelButton);
    btnLayout->addWidget(m_saveButton);
    m_mainLayout->addLayout(btnLayout);

    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_saveButton, &QPushButton::clicked, this, &FaceCaptureDialog::onSaveButtonClicked);

    QCameraDevice defaultCameraDevice = QMediaDevices::defaultVideoInput();
    if (defaultCameraDevice.isNull()) {
        m_videoLabel->setText("❌ 未检测到本地可用摄像头硬件！");
        qWarning() << "[Face Dialog] ❌ 失败：找不到默认视频输入设备。";
    } else {
        qDebug() << "[Face Dialog] 🔌 成功绑定摄像头驱动:" << defaultCameraDevice.description();

        m_camera = new QCamera(defaultCameraDevice, this);
        m_captureSession = new QMediaCaptureSession(this);
        m_videoSink = new QVideoSink(this);

        m_captureSession->setCamera(m_camera);
        m_captureSession->setVideoSink(m_videoSink);

        connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &FaceCaptureDialog::handleVideoFrame);

        m_camera->start();
    }
}

FaceCaptureDialog::~FaceCaptureDialog()
{
    if (m_camera) {
        m_camera->stop();
    }
}

void FaceCaptureDialog::onSaveButtonClicked()
{
    QString name = m_nameEdit->text().trimmed();
    QString workId = m_workIdEdit->text().trimmed();

    if (name.isEmpty() || workId.isEmpty()) {
        QMessageBox::warning(this, "警告", "请确保【姓名】与【工号/卡号】全部填写完整！");
        return;
    }

    // 检查是否有缓存的高质量人脸
    if (m_bestFaceROI.empty()) {
        QMessageBox::warning(this, "警告", "未检测到合格人脸，请确保：\n1. 人脸在画面中央\n2. 光线充足\n3. 保持稳定清晰");
        return;
    }

    qDebug() << "[Face Dialog] 📸 用户按下抓拍，使用缓存的高质量人脸...";

    // 将缓存的高质量人脸转为 QImage
    cv::Mat rgbFace;
    cv::cvtColor(m_bestFaceROI, rgbFace, cv::COLOR_BGR2RGB);
    QImage capturedImage(rgbFace.data, rgbFace.cols, rgbFace.rows, rgbFace.step, QImage::Format_RGB888);
    QImage finalImage = capturedImage.copy();

    QString folderPath = "faces";
    QDir dir;
    if (!dir.exists(folderPath)) {
        dir.mkdir(folderPath);
    }

    QString fileName = QString("%1/%2_%3.jpg")
                           .arg(folderPath)
                           .arg(workId)
                           .arg(QDateTime::currentMSecsSinceEpoch());

    if (!finalImage.save(fileName, "JPG", 90)) {
        QMessageBox::warning(this, "存储失败", "系统磁盘写入权限不足，无法保存照片！");
        return;
    }

    FaceDatabaseViewModel::instance().addUser(name, workId, fileName);

    QMessageBox::information(this, "实时采集成功", QString("用户【%1】白名单凭证已永久安全落库！").arg(name));

    this->accept();
}

void FaceCaptureDialog::handleVideoFrame(const QVideoFrame &frame)
{
    if (!frame.isValid()) return;

    m_currentFrame = frame;

    QVideoFrame cloneFrame(frame);
    if (!cloneFrame.map(QVideoFrame::ReadOnly)) {
        return;
    }

    QImage img = cloneFrame.toImage();
    cloneFrame.unmap();

    if (img.isNull()) return;

    QImage openCvImage = img.convertToFormat(QImage::Format_RGB888);
    
    cv::Mat matFrameSrc(openCvImage.height(), openCvImage.width(), CV_8UC3,
                        const_cast<uchar*>(openCvImage.bits()), openCvImage.bytesPerLine());
    cv::Mat matFrame = matFrameSrc.clone(); 
    cv::cvtColor(matFrame, matFrame, cv::COLOR_RGB2BGR);

    static int frameCounter = 0;
    static std::vector<cv::Rect> cachedFaces;
    frameCounter++;

    if (frameCounter >= 4) {
        cv::Mat gray;
        cv::cvtColor(matFrame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);

        cv::CascadeClassifier faceCascade;
        std::string xmlPath = "C:/my-github-project/EdgeXGuard/software/xml/haarcascade_frontalface_alt.xml";
        
        if (faceCascade.load(xmlPath)) {
            std::vector<cv::Rect> tempFaces;
            faceCascade.detectMultiScale(gray, tempFaces, 1.2, 3, 0, cv::Size(120, 120));
            
            if (!tempFaces.empty()) {
                cachedFaces = tempFaces;
            } else {
                cachedFaces.clear();
            }
        } else {
            static int logCount = 0;
            if (logCount++ % 60 == 0) {
                qDebug() << "🔴 [核心故障] 未找到模型文件:" << QString::fromStdString(xmlPath);
            }
        }
        frameCounter = 0; 
    }

    // =========================================================================
    // 🛡️ 人脸质量校验和最佳人脸缓存
    // =========================================================================
    bool hasGoodFace = false;
    std::string qualityReason;

    if (!cachedFaces.empty()) {
        for (const auto& rect : cachedFaces) {
            // 提取人脸 ROI
            cv::Mat faceROI = matFrame(rect).clone();
            
            // 质量校验
            if (isFaceQualityValid(faceROI, rect, matFrame.cols, matFrame.rows, qualityReason)) {
                hasGoodFace = true;
                
                // 更新最佳人脸（可简单按尺寸最大选，或按清晰度最高）
                int currentArea = rect.width * rect.height;
                int bestArea = m_bestFaceRect.width * m_bestFaceRect.height;
                if (currentArea > bestArea || m_bestFaceROI.empty()) {
                    m_bestFaceROI = faceROI.clone();
                    m_bestFaceRect = rect;
                    m_bestFaceQualityReason = QString::fromStdString(qualityReason);
                    qDebug() << "[Face Quality] ✅ 更新最佳人脸，尺寸:" << rect.width << "x" << rect.height;
                }
                
                // 绘制绿色边框（高质量）
                cv::rectangle(matFrame, rect, cv::Scalar(0, 255, 0), 3);
                cv::putText(matFrame, "QUALITY: GOOD", 
                            cv::Point(rect.x, rect.y - 10),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
            } else {
                // 质量不合格，绘制黄色边框
                cv::rectangle(matFrame, rect, cv::Scalar(0, 255, 255), 2);
                cv::putText(matFrame, qualityReason, 
                            cv::Point(rect.x, rect.y - 10),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
            }
        }
    }

    // 如果没有检测到人脸，显示引导信息
    if (!hasGoodFace) {
        cv::putText(matFrame, "WARNING: NO VALID FACE DETECTED", 
                    cv::Point(30, 50),                                 
                    cv::FONT_HERSHEY_SIMPLEX, 0.7,                      
                    cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
        
        // 显示质量提示
        if (!m_bestFaceQualityReason.isEmpty()) {
            cv::putText(matFrame, m_bestFaceQualityReason.toStdString(), 
                        cv::Point(30, 80),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
        }
        
        int cx = matFrame.cols / 2;
        int cy = matFrame.rows / 2;
        cv::rectangle(matFrame, cv::Point(cx - 100, cy - 120), cv::Point(cx + 100, cy + 120), 
                      cv::Scalar(0, 0, 180), 2, cv::LINE_AA);
    } else {
        // 有高质量人脸时，在画面顶部显示绿色状态
        cv::putText(matFrame, "FACE QUALITY: GOOD - READY TO CAPTURE", 
                    cv::Point(30, 50),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 1);
    }

    // 渲染
    cv::cvtColor(matFrame, matFrame, cv::COLOR_BGR2RGB);
    QImage resultImage(matFrame.data, matFrame.cols, matFrame.rows, matFrame.step, QImage::Format_RGB888);
    QImage finalCopy = resultImage.copy();

    QPixmap pixmap = QPixmap::fromImage(finalCopy).scaled(m_videoLabel->size(),
                                                           Qt::KeepAspectRatioByExpanding,
                                                           Qt::SmoothTransformation);

    QMetaObject::invokeMethod(m_videoLabel, [this, pixmap]() {
        m_videoLabel->setPixmap(pixmap);
    }, Qt::QueuedConnection);
}