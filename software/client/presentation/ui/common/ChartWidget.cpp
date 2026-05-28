#include "ChartWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
    , m_maxDataPoints(60)
    , m_minTemp(0)
    , m_maxTemp(50)
    , m_minHumi(0)
    , m_maxHumi(100)
    , m_showTemperature(true)
    , m_showHumidity(true)
    , m_title("温湿度历史曲线")
    , m_zoomLevel(1.0f)
    , m_offset(0)
    , m_isDragging(false)
    , m_needsRedraw(true)
{
    setMinimumHeight(250);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setStyleSheet("background-color: white; border-radius: 8px;");
}

void ChartWidget::setTemperatureData(const QVector<float>& data)
{
    m_tempData = data;
    if (m_tempData.size() > m_maxDataPoints) {
        m_tempData = m_tempData.mid(m_tempData.size() - m_maxDataPoints);
    }

    if (!m_tempData.isEmpty()) {
        m_minTemp = *std::min_element(m_tempData.begin(), m_tempData.end());
        m_maxTemp = *std::max_element(m_tempData.begin(), m_tempData.end());
        float range = m_maxTemp - m_minTemp;
        if (range < 5) range = 5;
        m_minTemp = std::max(0.0f, m_minTemp - range * 0.1f);
        m_maxTemp = m_maxTemp + range * 0.1f;
    }

    m_needsRedraw = true;
    update();
}

void ChartWidget::setHumidityData(const QVector<int>& data)
{
    m_humiData = data;
    qDebug() << "setHumidityData - size:" << m_humiData.size();
    if (!m_humiData.isEmpty()) {
        qDebug() << "First 5 values:" << m_humiData.first(5);
    }
    m_humiData = data;
    if (m_humiData.size() > m_maxDataPoints) {
        m_humiData = m_humiData.mid(m_humiData.size() - m_maxDataPoints);
    }

    if (!m_humiData.isEmpty()) {
        m_minHumi = *std::min_element(m_humiData.begin(), m_humiData.end());
        m_maxHumi = *std::max_element(m_humiData.begin(), m_humiData.end());
        float range = m_maxHumi - m_minHumi;
        if (range < 10) range = 10;
        m_minHumi = std::max(0, (int)(m_minHumi - range * 0.1f));
        m_maxHumi = std::min(100, (int)(m_maxHumi + range * 0.1f));
    }

    m_needsRedraw = true;
    update();
}

void ChartWidget::addTemperaturePoint(float value)
{
    m_tempData.append(value);
    if (m_tempData.size() > m_maxDataPoints) {
        m_tempData.removeFirst();
    }

    if (!m_tempData.isEmpty()) {
        m_minTemp = *std::min_element(m_tempData.begin(), m_tempData.end());
        m_maxTemp = *std::max_element(m_tempData.begin(), m_tempData.end());
        float range = m_maxTemp - m_minTemp;
        if (range < 5) range = 5;
        m_minTemp = std::max(0.0f, m_minTemp - range * 0.1f);
        m_maxTemp = m_maxTemp + range * 0.1f;
    }

    m_needsRedraw = true;
    update();
}

void ChartWidget::addHumidityPoint(int value)
{
    m_humiData.append(value);
    if (m_humiData.size() > m_maxDataPoints) {
        m_humiData.removeFirst();
    }

    if (!m_humiData.isEmpty()) {
        m_minHumi = *std::min_element(m_humiData.begin(), m_humiData.end());
        m_maxHumi = *std::max_element(m_humiData.begin(), m_humiData.end());
        float range = m_maxHumi - m_minHumi;
        if (range < 10) range = 10;
        m_minHumi = std::max(0, (int)(m_minHumi - range * 0.1f));
        m_maxHumi = std::min(100, (int)(m_maxHumi + range * 0.1f));
    }

    m_needsRedraw = true;
    update();
}

void ChartWidget::clearData()
{
    m_tempData.clear();
    m_humiData.clear();
    m_needsRedraw = true;
    update();
}

void ChartWidget::setMaxDataPoints(int maxPoints)
{
    m_maxDataPoints = maxPoints;
    setTemperatureData(m_tempData);
    setHumidityData(m_humiData);
}

void ChartWidget::showTemperature(bool show)
{
    m_showTemperature = show;
    m_needsRedraw = true;
    update();
}

void ChartWidget::showHumidity(bool show)
{
    m_showHumidity = show;
    m_needsRedraw = true;
    update();
}

void ChartWidget::setTitle(const QString& title)
{
    m_title = title;
    m_needsRedraw = true;
    update();
}

void ChartWidget::paintEvent(QPaintEvent *event)
{
    if (m_needsRedraw) {
        updatePixmap();
        m_needsRedraw = false;
    }

    QPainter painter(this);
    painter.drawPixmap(event->rect(), m_pixmap, event->rect());
}

void ChartWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_needsRedraw = true;
}

void ChartWidget::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() > 0 ? 1.1f : 0.9f;
    m_zoomLevel *= delta;
    m_zoomLevel = std::max(0.5f, std::min(5.0f, m_zoomLevel));
    m_needsRedraw = true;
    update();
}

void ChartWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastDragPos = event->pos();
    }
}

void ChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        int dx = event->pos().x() - m_lastDragPos.x();
        m_offset += dx;
        m_lastDragPos = event->pos();
        m_needsRedraw = true;
        update();
    }
}

void ChartWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
}

void ChartWidget::updatePixmap()
{
    if (width() <= 0 || height() <= 0) return;

    m_pixmap = QPixmap(size());
    m_pixmap.fill(Qt::white);

    QPainter painter(&m_pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int margin_left = 60;   // 增加左边距
    int margin_right = 50;  // 增加右边距
    int margin_top = 45;    // 增加上边距
    int margin_bottom = 35; // 增加下边距

    int chartWidth = width() - margin_left - margin_right;
    int chartHeight = height() - margin_top - margin_bottom;

    if (chartWidth <= 0 || chartHeight <= 0) return;

    // 绘制标题
    painter.setPen(Qt::black);
    QFont titleFont = painter.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRect(margin_left, 8, chartWidth, 25), Qt::AlignCenter, m_title);

    // 绘制网格
    drawGrid(painter, chartWidth, chartHeight);

    // ===== 绘制温度曲线（红色）=====
    if (m_showTemperature && m_tempData.size() >= 2) {
        QPainterPath path;
        bool first = true;
        for (int i = 0; i < m_tempData.size(); ++i) {
            float x = margin_left + ((float)i / (m_maxDataPoints - 1)) * chartWidth;
            float yPercent = (m_tempData[i] - m_minTemp) / (m_maxTemp - m_minTemp);
            yPercent = std::max(0.0f, std::min(1.0f, yPercent));
            float y = margin_top + (1.0f - yPercent) * chartHeight;

            if (first) {
                path.moveTo(x, y);
                first = false;
            } else {
                path.lineTo(x, y);
            }
        }
        painter.setPen(QPen(QColor(239, 68, 68), 2));
        painter.drawPath(path);
    }

    // ===== 绘制湿度曲线（蓝色）- 修复版 =====
    if (m_showHumidity && m_humiData.size() >= 2) {
        QPainterPath path;
        bool first = true;

        for (int i = 0; i < m_humiData.size(); ++i) {
            float x = margin_left + ((float)i / (m_maxDataPoints - 1)) * chartWidth;

            // 计算 Y 位置
            float range = m_maxHumi - m_minHumi;
            float yPercent = (m_humiData[i] - m_minHumi) / range;
            yPercent = std::max(0.0f, std::min(1.0f, yPercent));
            float y = margin_top + (1.0f - yPercent) * chartHeight;

            if (first) {
                path.moveTo(x, y);
                first = false;
            } else {
                path.lineTo(x, y);
            }
        }
        painter.setPen(QPen(QColor(59, 130, 246), 2));
        painter.drawPath(path);
    }

    drawLegend(painter, width(), height());
}

void ChartWidget::drawGrid(QPainter& painter, int width, int height)
{
    painter.save();

    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    // 垂直线（时间轴）
    for (int i = 0; i <= 6; ++i) {
        int x = 50 + i * width / 6;
        painter.setPen(QPen(QColor(230, 230, 230), 1));
        painter.drawLine(x, 40, x, 40 + height);

        int seconds = (m_maxDataPoints / 6) * i;
        painter.setPen(Qt::gray);
        painter.drawText(QRect(x - 20, 40 + height + 5, 40, 15), Qt::AlignCenter, QString("%1s").arg(seconds));
    }

    // 水平线（Y轴）
    for (int i = 0; i <= 4; ++i) {
        int y = 40 + i * height / 4;
        painter.setPen(QPen(QColor(230, 230, 230), 1));
        painter.drawLine(50, y, 50 + width, y);

        // 温度标签（左侧）
        float temp = m_maxTemp - (m_maxTemp - m_minTemp) * i / 4.0f;
        painter.setPen(QColor(239, 68, 68));
        painter.drawText(QRect(5, y - 8, 40, 16), Qt::AlignRight, QString("%1°C").arg(temp, 0, 'f', 1));

        // 湿度标签（右侧）
        int humi = m_maxHumi - (m_maxHumi - m_minHumi) * i / 4.0f;
        painter.setPen(QColor(59, 130, 246));
        painter.drawText(QRect(50 + width + 8, y - 8, 35, 16), Qt::AlignLeft, QString("%1%").arg(humi));
    }

    painter.setPen(QPen(QColor(150, 150, 150), 1));
    painter.drawRect(50, 40, width, height);

    painter.restore();
}

void ChartWidget::drawLegend(QPainter& painter, int width, int height)
{
    painter.save();

    int legendX = width - 120;
    int legendY = 8;
    int itemHeight = 18;

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 200));
    painter.drawRoundedRect(legendX - 5, legendY - 2, 115, itemHeight * 2 + 4, 4, 4);

    if (m_showTemperature) {
        painter.setPen(QPen(QColor(239, 68, 68), 2));
        painter.drawLine(legendX, legendY + 8, legendX + 20, legendY + 8);
        painter.setPen(Qt::black);
        painter.drawText(legendX + 25, legendY + 12, "温度");
    }

    if (m_showHumidity) {
        painter.setPen(QPen(QColor(59, 130, 246), 2));
        painter.drawLine(legendX, legendY + itemHeight + 8, legendX + 20, legendY + itemHeight + 8);
        painter.setPen(Qt::black);
        painter.drawText(legendX + 25, legendY + itemHeight + 12, "湿度");
    }

    painter.restore();
}

// 空实现（未使用的方法）
void ChartWidget::drawCurve(QPainter& painter, const QVector<QPointF>& points, const QColor& color)
{
    Q_UNUSED(painter)
    Q_UNUSED(points)
    Q_UNUSED(color)
}

void ChartWidget::calculateTransform(int width, int height)
{
    Q_UNUSED(width)
    Q_UNUSED(height)
}

QPointF ChartWidget::dataToPixel(float x, float y, int width, int height) const
{
    Q_UNUSED(width)
    Q_UNUSED(height)
    return QPointF(x, y);
}