#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QWidget>
#include <QLabel>
#include <QVector>

class ChartWidget;

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);

private slots:
    void updateSensorDisplay();

private:
    void setupUI();
    void createChartCard();

    // 传感器显示
    QLabel* m_tempLabel;
    QLabel* m_humidLabel;
    QLabel* m_pirLabel;

    // 设备状态
    QLabel* m_fanLabel;
    QLabel* m_lightLabel;

    // 图表
    ChartWidget* m_chartWidget;

    // 历史数据缓存
    QVector<float> m_tempHistory;
    QVector<int> m_humiHistory;
    int m_maxHistoryPoints;
};

#endif // DASHBOARDPAGE_H