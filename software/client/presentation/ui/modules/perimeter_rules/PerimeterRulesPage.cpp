#include "PerimeterRulesPage.h"
#include "presentation/viewmodel/perimeter_rules/PerimeterRulesViewModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QInputDialog>
#include <QDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QDialogButtonBox>

PerimeterRulesPage::PerimeterRulesPage(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 模块大标题
    QLabel* titleLabel = new QLabel("周界规则引擎与边缘决策联动策略配置", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #f8fafc; margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);

    // 绑定大管家模型
    m_tableView = new QTableView(this);
    m_tableView->setModel(&PerimeterRulesViewModel::instance());
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableView->setStyleSheet("QTableView { background-color: #0f172a; color: #e2e8f0; gridline-color: #334155; }");
    mainLayout->addWidget(m_tableView, 1);

    // 底部控制按钮区
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_addBtn = new QPushButton("➕ 创建联动规则", this);
    m_toggleBtn = new QPushButton("🔄 切换启用/禁用", this);
    m_deleteBtn = new QPushButton("🗑️ 强行删除规则", this);

    m_addBtn->setStyleSheet("background-color: #3b82f6; color: white; padding: 8px 15px; border-radius: 6px; font-weight: bold;");
    m_toggleBtn->setStyleSheet("background-color: #475569; color: white; padding: 8px 15px; border-radius: 6px;");
    m_deleteBtn->setStyleSheet("background-color: #ef4444; color: white; padding: 8px 15px; border-radius: 6px;");

    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_toggleBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_deleteBtn);
    mainLayout->addLayout(btnLayout);

    // 事件信号连接
    connect(m_addBtn, &QPushButton::clicked, this, &PerimeterRulesPage::onAddRuleClicked);
    connect(m_toggleBtn, &QPushButton::clicked, this, &PerimeterRulesPage::onToggleStatusClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &PerimeterRulesPage::onDeleteClicked);
}

void PerimeterRulesPage::onAddRuleClicked() {
    // 弹出一个精美的工业风 IF-THEN 选择对话框
    QDialog dialog(this);
    dialog.setWindowTitle("新建边缘核心联动规则");
    dialog.setStyleSheet("QDialog { background-color: #1e293b; color: white; } QLabel { color: white; }");

    QFormLayout form(&dialog);

    QLineEdit* nameInput = new QLineEdit(&dialog);
    nameInput->setText("自定义安全联动规则");
    form.addRow("规则识别名称:", nameInput);

    // 【IF 触发源】下拉列表
    QComboBox* triggerCombo = new QComboBox(&dialog);
    triggerCombo->addItems({"AI: 检测到未知陌生人", "AI: 检测到特定区域有人员聚集", "传感器: PIR红外人体触发", "传感器: 环境温度 > 50℃", "传感器: 烟雾浓度超标警报"});
    form.addRow("【IF】当系统触发事件时:", triggerCombo);

    // 【THEN 联动动作】下拉列表
    QComboBox* actionCombo = new QComboBox(&dialog);
    actionCombo->addItems({"硬件: 驱使本地内核PWM舵机锁门", "硬件: 分布式MQTT蜂鸣器长鸣", "硬件: 启动GPIO本地排风扇", "业务: 本地录像流紧急抓拍并加锁", "业务: 向分布式MQTT总线发布特级强闯告警"});
    form.addRow("【THEN】执行智能联动动作:", actionCombo);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        if (!nameInput->text().isEmpty()) {
            PerimeterRulesViewModel::instance().addRule(nameInput->text(), triggerCombo->currentText(), actionCombo->currentText());
        }
    }
}

void PerimeterRulesPage::onToggleStatusClicked() {
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (!selected.isEmpty()) {
        PerimeterRulesViewModel::instance().toggleRuleStatus(selected.first().row());
    }
}

void PerimeterRulesPage::onDeleteClicked() {
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (!selected.isEmpty()) {
        PerimeterRulesViewModel::instance().removeRule(selected.first().row());
    }
}