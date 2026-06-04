#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QWidget>
#include <QLabel>
#include <QList>

class ChartWidget;

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);
    ~DashboardPage() = default;

private slots:
    // 🟢 响应式核心：ViewModel 数据一变，立刻物理刷新此槽
    void updateSensorDisplay();

private:
    void setupUI();
    void createChartCard();

    // UI 控件指针
    QLabel* m_tempLabel = nullptr;
    QLabel* m_humidLabel = nullptr;
    QLabel* m_pirLabel = nullptr;
    QLabel* m_fanLabel = nullptr;
    QLabel* m_lightLabel = nullptr;
    ChartWidget* m_chartWidget = nullptr;

    // 历史时序队列
    QList<float> m_tempHistory;
    QList<int>   m_humiHistory;
    int m_maxHistoryPoints;
};

#endif // DASHBOARDPAGE_H