#include "facecapturedialog.h"
#include "presentation/viewmodel/face_database/FaceDatabaseViewModel.h" // 🟢 引入数据大管家
#include <QMessageBox>
#include <QDebug>
#include <QMediaDevices>
#include <QImage>
#include <QPixmap>
#include <QDateTime>
#include <QDir>

// 🟢 物理引入 OpenCV 4100 核心三大算法头文件
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

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

    // 获取电脑当前默认的可用摄像头硬件
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

// =========================================================================
// 💾 【抓拍按钮事件】：只负责拦截空输入与安全落盘，不再重复做繁重的检测逻辑
// =========================================================================
void FaceCaptureDialog::onSaveButtonClicked()
{
    QString name = m_nameEdit->text().trimmed();
    QString workId = m_workIdEdit->text().trimmed();

    // 1. 拦截空输入
    if (name.isEmpty() || workId.isEmpty()) {
        QMessageBox::warning(this, "警告", "请确保【姓名】与【工号/卡号】全部填写完整！");
        return;
    }

    // 2. 拦截无效硬件帧
    if (!m_currentFrame.isValid()) {
        QMessageBox::warning(this, "警告", "摄像头尚未捕获到有效视频帧，请稍后再试！");
        return;
    }

    qDebug() << "[Face Dialog] 📸 用户按下抓拍，正在安全落盘与同步持久化...";

    QImage capturedImage = m_currentFrame.toImage();
    if (capturedImage.isNull()) {
        QMessageBox::warning(this, "录入失败", "视频帧图层提取失败！");
        return;
    }

    // 3. 创建物理照片缓存目录
    QString folderPath = "faces";
    QDir dir;
    if (!dir.exists(folderPath)) {
        dir.mkdir(folderPath);
    }

    // 4. 生成物理照片文件路径
    QString fileName = QString("%1/%2_%3.jpg")
                           .arg(folderPath)
                           .arg(workId)
                           .arg(QDateTime::currentMSecsSinceEpoch());

    // 5. 将无污染的原始抓拍照片落盘
    if (!capturedImage.save(fileName)) {
        QMessageBox::warning(this, "存储失败", "系统磁盘写入权限不足，无法保存照片！");
        return;
    }

    // 6. 物理直通车：同步持久化刷新本地 JSON 账本
    FaceDatabaseViewModel::instance().addUser(name, workId, fileName);

    QMessageBox::information(this, "实时采集成功", QString("用户【%1】白名单凭证已永久安全落库！").arg(name));

    this->accept(); // 关闭弹窗，刷新主表
}
// =========================================================================
// 🟢 【日常动态流拦截】：在每秒 30 帧的硬件流里实时进行人脸检测并绘制绿框
// =========================================================================
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

    // 1. 强制进行图层格式对齐
    QImage openCvImage = img.convertToFormat(QImage::Format_RGB888);
    
    // 2. 物理映射至 OpenCV 矩阵
    cv::Mat matFrame(openCvImage.height(), openCvImage.width(), CV_8UC3,
                     const_cast<uchar*>(openCvImage.bits()), openCvImage.bytesPerLine());
    cv::cvtColor(matFrame, matFrame, cv::COLOR_RGB2BGR);

    // 3. 🪐【核心性能补丁：工业级跳帧拦截】
    // 声明静态计数器与坐标缓存，每 4 帧才允许 CPU 执行一次沉重的人脸识别核心算法
    static int frameCounter = 0;
    static std::vector<cv::Rect> cachedFaces;
    frameCounter++;

    if (frameCounter % 4 == 0) {
        cv::Mat gray;
        cv::cvtColor(matFrame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);

        cv::CascadeClassifier faceCascade;
        if (faceCascade.load("haarcascade_frontalface_alt.xml")) {
            // 适当调大比例因子(1.2)并限制最小尺寸(120x120)，再次把推理开销腰斩
            faceCascade.detectMultiScale(gray, cachedFaces, 1.2, 3, 0, cv::Size(120, 120));
        }
        frameCounter = 0; // 计数器归零
    }

    // 4. 🪐【动态业务状态分流渲染机制】
    if (!cachedFaces.empty()) {
        // A 状态：画面中捕获到人类正脸 ──► 绘制科技感绿色加粗追踪框
        for (const auto& rect : cachedFaces) {
            cv::rectangle(matFrame, rect, cv::Scalar(0, 255, 0), 3); 
        }
    } else {
        // B 状态：画面无脸 ──► 直接在视频流像素矩阵上强行烙印动态红色半透明警告文案！
        // 渲染文案: "WARNING: NO FACE DETECTED"
        cv::putText(matFrame, "WARNING: NO FACE DETECTED", 
                    cv::Point(30, 50),                         // 坐标点 (X=30, Y=50) 
                    cv::FONT_HERSHEY_SIMPLEX, 0.8,              // 字体与字号
                    cv::Scalar(0, 0, 255), 2, cv::LINE_AA);    // 纯红色彩 (BGR: 0, 0, 255)，粗细为2
        
        // 顺手在正中央画一个半透明的红色引导虚线框，引导用户把脸凑过来
        int cx = matFrame.cols / 2;
        int cy = matFrame.rows / 2;
        cv::rectangle(matFrame, cv::Point(cx - 80, cy - 100), cv::Point(cx + 80, cy + 100), 
                      cv::Scalar(0, 0, 180), 1, cv::LINE_AA);
    }

    // 5. 渲染完特效后，将 BGR 矩阵安全转换回 Qt QImage 投喂给主界面
    cv::cvtColor(matFrame, matFrame, cv::COLOR_BGR2RGB);
    QImage resultImage(matFrame.data, matFrame.cols, matFrame.rows, matFrame.step, QImage::Format_RGB888);

    QPixmap pixmap = QPixmap::fromImage(resultImage).scaled(m_videoLabel->size(),
                                                            Qt::KeepAspectRatioByExpanding,
                                                            Qt::SmoothTransformation);

    QMetaObject::invokeMethod(m_videoLabel, [this, pixmap]() {
        m_videoLabel->setPixmap(pixmap);
    }, Qt::QueuedConnection);
}