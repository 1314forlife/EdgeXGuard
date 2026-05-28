#include "FaceDatabasePage.h"
#include "presentation/viewmodel/face_database/FaceDatabaseViewModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>

FaceDatabasePage::FaceDatabasePage(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel* titleLabel = new QLabel("底端人脸凭证白名单库管理", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #f8fafc; margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);

    // 绑定刚才写好的假数据大管家
    m_tableView = new QTableView(this);
    m_tableView->setModel(&FaceDatabaseViewModel::instance());
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableView->setStyleSheet("QTableView { background-color: #0f172a; color: #e2e8f0; gridline-color: #334155; }");
    mainLayout->addWidget(m_tableView, 1);

    // 底部操作按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_refreshBtn = new QPushButton("重置/刷新模拟数据", this);
    m_deleteBtn = new QPushButton("注销选中凭证", this);

    m_refreshBtn->setStyleSheet("background-color: #1e293b; color: white; padding: 8px 15px; border-radius: 6px;");
    m_deleteBtn->setStyleSheet("background-color: #ef4444; color: white; padding: 8px 15px; border-radius: 6px;");

    btnLayout->addWidget(m_refreshBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_deleteBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_refreshBtn, &QPushButton::clicked, &FaceDatabaseViewModel::instance(), &FaceDatabaseViewModel::loadMockData);
    connect(m_deleteBtn, &QPushButton::clicked, this, &FaceDatabasePage::onDeleteClicked);
}

void FaceDatabasePage::onDeleteClicked() {
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (!selected.isEmpty()) {
        FaceDatabaseViewModel::instance().removeUser(selected.first().row());
    }
}