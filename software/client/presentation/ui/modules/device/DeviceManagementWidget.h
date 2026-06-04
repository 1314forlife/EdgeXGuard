#ifndef DEVICEMANAGEMENTWIDGET_H
#define DEVICEMANAGEMENTWIDGET_H

#include <QWidget>
#include <QSplitter>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QGridLayout>

class DeviceManagementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceManagementWidget(QWidget *parent = nullptr);

private slots:
    void onDeviceSelected(QTreeWidgetItem* item, int column);

    // 新增：PTZ 控制槽函数
    void onPtzLeft();
    void onPtzRight();
    void onPtzUp();
    void onPtzDown();
    void onPtzStop();
    void onPtzHome();
    void onSpeedChanged(int value);
    void onPtzMoveTimeout();  // 定时器超时，发送停止命令

private:
    void setupUI();
    void setupDeviceTree();
    void setupDetailPanel();
    void setupPtzPanel(QVBoxLayout* detailLayout);  // 新增：创建 PTZ 面板
    void loadDevices();

    // 获取当前选中设备的 serviceAddress（ONVIF 服务地址）
    QString getCurrentServiceAddress();

    QLineEdit* m_nameEdit;
    QLineEdit* m_typeEdit;
    QLineEdit* m_statusEdit;
    QLineEdit* m_rtspEdit;

    QSplitter* m_splitter;
    QTreeWidget* m_deviceTree;
    QStackedWidget* m_detailStack;

    //PTZ 相关成员
    QSlider* m_speedSlider;      // 速度滑块 (0.1 - 1.0)
    QLabel* m_speedLabel;        // 速度显示
    QTimer* m_ptzTimer;          // 定时器，实现按住移动、松开停止
    bool m_isMoving;             // 是否正在移动
    QString m_currentServiceAddress;  // 当前设备的 ONVIF 服务地址
};

#endif