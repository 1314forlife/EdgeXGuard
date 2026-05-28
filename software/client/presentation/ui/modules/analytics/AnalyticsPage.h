#ifndef ANALYTICS_PAGE_H
#define ANALYTICS_PAGE_H

#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <memory>
#include "application/service/detection/DetectionService.h"

class AnalyticsPage : public QWidget {
    Q_OBJECT
public:
    explicit AnalyticsPage(std::shared_ptr<DetectionService> detectionService, QWidget* parent = nullptr);
    ~AnalyticsPage() = default;

private slots:
    void onMockVerifyClicked();             // 响应用户点击“模拟刷脸”按钮
    void onDoorOpened(const QString& name);    // 接收Service层比对成功的信号
    void onStrangerAlert();                 // 接收Service层发现陌生人的信号

private:
    std::shared_ptr<DetectionService> m_detectionService; // 注入的业务大脑
    QPushButton* m_mockVerifyBtn;
    QTextEdit* m_logConsole;                // 炫酷的黑绿业务控制台
};

#endif // ANALYTICS_PAGE_H