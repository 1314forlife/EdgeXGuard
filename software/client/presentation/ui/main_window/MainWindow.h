#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QHash>
#include <memory>

class SideBar;
class DetectionService;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onModuleChanged(const QString& module);

private:
    void setupUI();
    void setupMenu();
    void setupCentralArea();
    void startDataSync();
    void updateSideBarData();  // 新增：单独提取数据更新逻辑
    void applyStyle();

    SideBar* m_sideBar;
    std::shared_ptr<DetectionService> m_detectionService;
    QStackedWidget* m_stackedWidget;
    QHash<QString, int> m_moduleIndexMap;  // 新增：模块名到索引的映射
};

#endif // MAINWINDOW_H