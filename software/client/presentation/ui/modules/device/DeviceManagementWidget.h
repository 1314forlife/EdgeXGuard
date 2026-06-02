#ifndef DEVICEMANAGEMENTWIDGET_H
#define DEVICEMANAGEMENTWIDGET_H

#include <QWidget>
#include <QSplitter>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>

class DeviceManagementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceManagementWidget(QWidget *parent = nullptr);
private slots:
    void onDeviceSelected(QTreeWidgetItem* item, int column);

private:
    void setupUI();
    void setupDeviceTree();
    void setupDetailPanel();
    void loadDevices();

    QLineEdit* m_nameEdit;
    QLineEdit* m_typeEdit;
    QLineEdit* m_statusEdit;
    QLineEdit* m_rtspEdit;

    QSplitter* m_splitter;
    QTreeWidget* m_deviceTree;
    QStackedWidget* m_detailStack;
};

#endif