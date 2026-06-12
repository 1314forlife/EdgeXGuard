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

    // ─────────────────────────────────────────────────────────
    // 【1. 标题与头部布局】
    // ─────────────────────────────────────────────────────────
    QLabel* titleLabel = new QLabel("底端人脸凭证白名单库管理", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #f8fafc;");
    mainLayout->addWidget(titleLabel);

    // ─────────────────────────────────────────────────────────
    // 🚀【2. 新增：性能加速中控面板区】（插在标题与表格中间）
    // ─────────────────────────────────────────────────────────
    QHBoxLayout* controlPanelLayout = new QHBoxLayout();
    controlPanelLayout->setContentsMargins(0, 10, 0, 10);

    // 2.1 算法加速控制开关
    m_hnswCheckBox = new QCheckBox("启用 2.0 HNSW 高维向量拓扑检索加速", this);
    m_hnswCheckBox->setChecked(FaceDatabaseViewModel::instance().useHnswMode());
    m_hnswCheckBox->setStyleSheet(
        "QCheckBox { color: #94a3b8; font-size: 14px; font-weight: bold; spacing: 8px; }"
        "QCheckBox::indicator { width: 18px; height: 18px; }"
        "QCheckBox:hover { color: #38bdf8; }" // 鼠标悬停变天蓝色
    );
    controlPanelLayout->addWidget(m_hnswCheckBox);
    controlPanelLayout->addStretch();

    // 2.2 实时性能多维数据看板
    m_performanceLabel = new QLabel("当前策略: 1.0 暴力扫表遍历 | 实时检索耗时: ──", this);
    m_performanceLabel->setAlignment(Qt::AlignCenter);
    m_performanceLabel->setStyleSheet(
        "QLabel { "
        "  background-color: #1e293b; "  // 深蓝灰色工业背景
        "  color: #f43f5e; "             // 默认传统遍历用警告红
        "  font-family: 'Consolas', monospace; " // 等宽字体显专业
        "  font-size: 13px; "
        "  font-weight: bold; "
        "  padding: 6px 15px; "
        "  border: 1px solid #334155; "
        "  border-radius: 6px; "
        "}"
    );
    controlPanelLayout->addWidget(m_performanceLabel);
    mainLayout->addLayout(controlPanelLayout);

    // ─────────────────────────────────────────────────────────
    // 【3. 初始化表格视图并绑定单例模型】
    // ─────────────────────────────────────────────────────────
    m_tableView = new QTableView(this);
    m_tableView->setModel(&FaceDatabaseViewModel::instance());
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableView->setStyleSheet("QTableView { background-color: #0f172a; color: #e2e8f0; gridline-color: #334155; }");
    mainLayout->addWidget(m_tableView, 1);

    // ─────────────────────────────────────────────────────────
    // 【4. 底部操作按钮群】
    // ─────────────────────────────────────────────────────────
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

    // ─────────────────────────────────────────────────────────
    // 🚀【5. 信号与槽安全绑定】
    // ─────────────────────────────────────────────────────────
    // 原有核心功能连接
    connect(m_refreshBtn, &QPushButton::clicked, &FaceDatabaseViewModel::instance(), &FaceDatabaseViewModel::loadMockData);
    connect(m_deleteBtn, &QPushButton::clicked, this, &FaceDatabasePage::onDeleteClicked);
    connect(m_captureBtn, &QPushButton::clicked, this, &FaceDatabasePage::onCaptureClicked);

    // 🚀 新增联动：将开关状态无缝同步给 ViewModel 的控制属性
    connect(m_hnswCheckBox, &QCheckBox::toggled, &FaceDatabaseViewModel::instance(), &FaceDatabaseViewModel::setUseHnswMode);

    // 🚀 新增联动：死锁 ViewModel 弹射回来的性能测量信号，动态刷新 UI 看板样式与文字
    connect(&FaceDatabaseViewModel::instance(), &FaceDatabaseViewModel::performanceMetricsReady, this, 
            [=](double timeCountUs, const QString& modeName) {
                if (modeName == "HNSW") {
                    // HNSW 拓扑图分支：文字变绿，显示微秒
                    m_performanceLabel->setText(QString("当前策略: 2.0 HNSW 拓扑加速 | 实时检索耗时: %1 μs").arg(timeCountUs, 0, 'f', 1));
                    m_performanceLabel->setStyleSheet(
                        "QLabel { background-color: #064e3b; color: #34d399; font-family: 'Consolas'; font-size: 13px; font-weight: bold; padding: 6px 15px; border: 1px solid #059669; border-radius: 6px; }"
                    );
                } else {
                    // 传统 BruteForce 分支：文字变红，显示毫秒
                    double timeCountMs = timeCountUs / 1000.0;
                    m_performanceLabel->setText(QString("当前策略: 1.0 暴力扫表遍历 | 实时检索耗时: %1 ms").arg(timeCountMs, 0, 'f', 2));
                    m_performanceLabel->setStyleSheet(
                        "QLabel { background-color: #4c0519; color: #fb7185; font-family: 'Consolas'; font-size: 13px; font-weight: bold; padding: 6px 15px; border: 1px solid #e11d48; border-radius: 6px; }"
                    );
                }
            });
}

void FaceDatabasePage::onDeleteClicked() {
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (!selected.isEmpty()) {
        int targetRow = selected.first().row();
        qDebug() << "[UI] 🗑️ 申请注销当前选中行:" << targetRow;
        FaceDatabaseViewModel::instance().removeUser(targetRow);
        FaceDatabaseViewModel::instance().loadMockData();
    }
}

void FaceDatabasePage::onCaptureClicked() {
    qDebug() << "[UI] 🛎️ 准备唤醒采集仓...";
    FaceCaptureDialog dialog(this);
    dialog.setStyleSheet(this->styleSheet());

    if (dialog.exec() == QDialog::Accepted) {
        qDebug() << "[UI] 📥 采集仓业务成功闭环，主页准备同步更新视图...";
        FaceDatabaseViewModel::instance().loadMockData();
    }
}