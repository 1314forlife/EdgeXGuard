#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QPixmap>
class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChartWidget(QWidget *parent = nullptr);

    // 设置数据
    void setTemperatureData(const QVector<float>& data);
    void setHumidityData(const QVector<int>& data);
    void addTemperaturePoint(float value);
    void addHumidityPoint(int value);
    void clearData();

    // 设置显示范围（数据点个数）
    void setMaxDataPoints(int maxPoints);

    // 切换曲线显示
    void showTemperature(bool show);
    void showHumidity(bool show);

    // 设置标题
    void setTitle(const QString& title);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updatePixmap();
    void drawGrid(QPainter& painter, int width, int height);
    void drawCurve(QPainter& painter, const QVector<QPointF>& points, const QColor& color);
    void drawLegend(QPainter& painter, int width, int height);
    void calculateTransform(int width, int height);
    QPointF dataToPixel(float x, float y, int width, int height) const;

    // 数据
    QVector<float> m_tempData;
    QVector<int> m_humiData;
    int m_maxDataPoints;
    float m_minTemp;
    float m_maxTemp;
    int m_minHumi;
    int m_maxHumi;

    // 显示控制
    bool m_showTemperature;
    bool m_showHumidity;
    QString m_title;

    // 交互（缩放/平移）
    float m_zoomLevel;
    int m_offset;  // 平移偏移量
    bool m_isDragging;
    QPoint m_lastDragPos;

    // 双缓冲
    QPixmap m_pixmap;
    bool m_needsRedraw;
};

#endif // CHARTWIDGET_H