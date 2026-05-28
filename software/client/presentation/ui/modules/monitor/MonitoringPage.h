#ifndef MONITORINGPAGE_H
#define MONITORINGPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

// 前向声明
class OpenGLRenderer;
class RtmpPlayer;

class MonitoringPage : public QWidget
{
    Q_OBJECT

public:
    explicit MonitoringPage(QWidget* parent = nullptr);
    ~MonitoringPage();

private slots:
    void onOpenStream();
    void onStopStream();

private:
    void setupUI();

    QLineEdit* m_urlEdit;
    QPushButton* m_openBtn;
    QPushButton* m_stopBtn;
    QVBoxLayout* m_layout;

    QLabel* m_statusLabel;

    OpenGLRenderer* m_renderer;
    RtmpPlayer* m_player;
};

#endif