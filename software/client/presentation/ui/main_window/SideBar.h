#pragma once
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QButtonGroup>

class SideBar : public QWidget {
    Q_OBJECT
public:
    explicit SideBar(QWidget *parent = nullptr);

signals:
    void menuSelected(const QString &module);  // 通知 MainWindow 切换页面

public slots:
    void updateTemperature(float temp);
    void updateHumidity(int humi);
    void updateHumanDetected(bool detected);
    void updateFanStatus(bool on);
    void updateLightStatus(bool on);

private:
    void setupUI();
    void addNavButton(const QString &text, const QString &module);

    QButtonGroup *m_buttonGroup;
    QLabel *m_tempLabel;
    QLabel *m_humiLabel;
    QLabel *m_humanLabel;
    QLabel *m_fanLabel;
    QLabel *m_lightLabel;
};