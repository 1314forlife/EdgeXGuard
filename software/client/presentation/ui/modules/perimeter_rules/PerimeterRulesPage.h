#ifndef PERIMETER_RULES_PAGE_H
#define PERIMETER_RULES_PAGE_H

#include <QWidget>
#include <QTableView>
#include <QPushButton>

class PerimeterRulesPage : public QWidget {
    Q_OBJECT
public:
    explicit PerimeterRulesPage(QWidget* parent = nullptr);
private slots:
    void onAddRuleClicked();
    void onDeleteClicked();
    void onToggleStatusClicked();
private:
    QTableView* m_tableView;
    QPushButton* m_addBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_toggleBtn;
};

#endif // PERIMETER_RULES_PAGE_H