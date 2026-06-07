#include "FaceDatabasePage.h"
#include "facecapturedialog.h"
#include "presentation/viewmodel/face_database/FaceDatabaseViewModel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QDebug>

FaceDatabasePage::FaceDatabasePage(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel* titleLabel = new QLabel("底端人脸凭证白名单库管理", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #f8fafc; margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);

    // 1. 初始化表格视图并绑定全局唯一的 ViewModel 单例模型
    m_tableView = new QTableView(this);
    m_tableView->setModel(&FaceDatabaseViewModel::instance());
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableView->setStyleSheet("QTableView { background-color: #0f172a; color: #e2e8f0; gridline-color: #334155; }");
    mainLayout->addWidget(m_tableView, 1);

    // 2. 底部操作按钮群
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_refreshBtn = new QPushButton("重置/刷新模拟数据", this);
    m_captureBtn = new QPushButton("📷 实时录入人脸", this);
    m_deleteBtn = new QPushButton("注销选中凭证", this);

    m_refreshBtn->setStyleSheet("background-color: #1e293b; color: white; padding: 8px 15px; border-radius: 6px;");
    m_captureBtn->setStyleSheet("background-color: #2563eb; color: white; padding: 8px 15px; border-radius: 6px; font-weight: bold;");
    m_deleteBtn->setStyleSheet("background-color: #ef4444; color: white; padding: 8px 15px; border-radius: 6px;");

    btnLayout->addWidget(m_refreshBtn);
    btnLayout->addWidget(m_captureBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_deleteBtn);
    mainLayout->addLayout(btnLayout);

    // 3. 信号与槽安全绑定
    connect(m_refreshBtn, &QPushButton::clicked, &FaceDatabaseViewModel::instance(), &FaceDatabaseViewModel::loadMockData);
    connect(m_deleteBtn, &QPushButton::clicked, this, &FaceDatabasePage::onDeleteClicked);
    connect(m_captureBtn, &QPushButton::clicked, this, &FaceDatabasePage::onCaptureClicked);
}

// 🎯 业务逻辑：注销选中的白名单用户
void FaceDatabasePage::onDeleteClicked() {
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (!selected.isEmpty()) {
        int targetRow = selected.first().row();
        qDebug() << "[UI] 🗑️ 申请注销当前选中行:" << targetRow;

        // 调起数据大管家移除该行，ViewModel 内部会自动覆写本地 JSON 文件
        FaceDatabaseViewModel::instance().removeUser(targetRow);

        // 物理刷新，确保 UI 视图和本地硬盘状态绝对一致
        FaceDatabaseViewModel::instance().loadMockData();
    }
}

// 🎯 业务逻辑：唤醒人脸采集弹窗仓
void FaceDatabasePage::onCaptureClicked() {
    qDebug() << "[UI] 🛎️ 准备唤醒采集仓...";

    FaceCaptureDialog dialog(this);
    dialog.setStyleSheet(this->styleSheet());

    // 🪐 【核心解耦点】
    // 当用户在弹窗里点击拍照，且通过了 OpenCV 的“人脸防空拦截”后，
    // 弹窗内部会自己把图片存盘、调用 ViewModel 写入内存并同步刷新本地文件，最后返回 QDialog::Accepted。
    if (dialog.exec() == QDialog::Accepted) {

        qDebug() << "[UI] 📥 采集仓业务成功闭环，主页准备同步更新视图...";

        // 核心安全优化：主页面绝对不进行二次重复录入（杜绝数据重合）
        // 仅仅通过刷新直通车，让刚刚写入硬盘的新人员信息立刻同步闪现上屏！
        FaceDatabaseViewModel::instance().loadMockData();
    }
}