#ifndef FACE_DATABASE_PAGE_H
#define FACE_DATABASE_PAGE_H

#include <QWidget>
#include <QTableView>
#include <QPushButton>

class FaceDatabasePage : public QWidget {
    Q_OBJECT
public:
    explicit FaceDatabasePage(QWidget* parent = nullptr);
private slots:
    void onDeleteClicked(); // 点击删除按钮
private:
    QTableView* m_tableView;
    QPushButton* m_deleteBtn;
    QPushButton* m_refreshBtn;
};

#endif // FACE_DATABASE_PAGE_H