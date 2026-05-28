#include "PerimeterRulesViewModel.h"
#include <QDateTime>

PerimeterRulesViewModel& PerimeterRulesViewModel::instance() {
    static PerimeterRulesViewModel instance;
    return instance;
}

PerimeterRulesViewModel::PerimeterRulesViewModel(QObject* parent) : QAbstractTableModel(parent) {
    loadMockRules();
}

void PerimeterRulesViewModel::loadMockRules() {
    beginResetModel();
    m_mockRules.clear();

    // 预填三条极具工业背景的联动假数据
    m_mockRules.append({{"id", 1}, {"name", "防区A陌生人强闯警报"}, {"trigger", "AI: 检测到未知陌生人"}, {"action", "硬件: 分布式MQTT蜂鸣器长鸣"}, {"status", "已启用"}, {"time", "2026-05-20"}});
    m_mockRules.append({{"id", 2}, {"name", "夜间红外防盗锁门"}, {"trigger", "传感器: PIR红外人体触发"}, {"action", "硬件: 驱使本地内核PWM舵机锁门"}, {"status", "已启用"}, {"time", "2026-05-22"}});
    m_mockRules.append({{"id", 3}, {"name", "室内超温消防联动"}, {"trigger", "传感器: 环境温度 > 50℃"}, {"action", "业务: 开启本地风扇并上报云端"}, {"status", "已禁用"}, {"time", "2026-05-25"}});
    endResetModel();
}

int PerimeterRulesViewModel::rowCount(const QModelIndex &) const { return m_mockRules.size(); }
int PerimeterRulesViewModel::columnCount(const QModelIndex &) const { return 6; } // ID, 规则名称, 触发源, 联动动作, 状态, 创建时间

QVariant PerimeterRulesViewModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) return QVariant();

    const auto& rule = m_mockRules[index.row()];
    switch (index.column()) {
    case 0: return rule["id"];
    case 1: return rule["name"];
    case 2: return rule["trigger"];
    case 3: return rule["action"];
    case 4: return rule["status"];
    case 5: return rule["time"];
    }
    return QVariant();
}

QVariant PerimeterRulesViewModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) return QVariant();
    QStringList headers = {"规则ID", "规则名称", "【IF】事件触发源", "【THEN】智能联动动作", "运行状态", "创建时间"};
    return headers[section];
}

void PerimeterRulesViewModel::addRule(const QString& name, const QString& trigger, const QString& action) {
    int newId = m_mockRules.isEmpty() ? 1 : m_mockRules.last()["id"].toInt() + 1;
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    beginInsertRows(QModelIndex(), m_mockRules.size(), m_mockRules.size());
    m_mockRules.append({
        {"id", newId}, {"name", name}, {"trigger", trigger}, {"action", action}, {"status", "已启用"}, {"time", currentTime}
    });
    endInsertRows();
}

void PerimeterRulesViewModel::removeRule(int row) {
    if (row < 0 || row >= m_mockRules.size()) return;
    beginRemoveRows(QModelIndex(), row, row);
    m_mockRules.removeAt(row);
    endRemoveRows();
}

void PerimeterRulesViewModel::toggleRuleStatus(int row) {
    if (row < 0 || row >= m_mockRules.size()) return;
    QString currentStatus = m_mockRules[row]["status"].toString();
    m_mockRules[row]["status"] = (currentStatus == "已启用") ? "已禁用" : "已启用";
    emit dataChanged(index(row, 4), index(row, 4), {Qt::DisplayRole});
}