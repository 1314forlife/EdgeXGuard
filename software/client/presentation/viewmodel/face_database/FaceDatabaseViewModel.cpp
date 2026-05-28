#include "FaceDatabaseViewModel.h"
#include <QDateTime>

// ==================== 【★核心联动：建立中央全局假数据库】 ====================
// 让整个软件所有模块都能实时访问这同一个白名单数据源
static QList<QVariantMap>& getCentralMockDatabase() {
    static QList<QVariantMap> s_database;
    if (s_database.isEmpty()) {
        s_database.append({{"id", 1}, {"name", "张三"}, {"workId", "HQ-9527"}, {"privilege", "允许通行"}, {"time", "2026-05-20"}});
        s_database.append({{"id", 2}, {"name", "李四"}, {"workId", "HQ-8848"}, {"privilege", "允许通行"}, {"time", "2026-05-22"}});
        s_database.append({{"id", 3}, {"name", "王五"}, {"workId", "HQ-1024"}, {"privilege", "禁止通行"}, {"time", "2026-05-25"}});
    }
    return s_database;
}
// =========================================================================

FaceDatabaseViewModel& FaceDatabaseViewModel::instance() {
    static FaceDatabaseViewModel instance;
    return instance;
}

FaceDatabaseViewModel::FaceDatabaseViewModel(QObject* parent) : QAbstractTableModel(parent) {
    loadMockData();
}

void FaceDatabaseViewModel::loadMockData() {
    beginResetModel();
    // 每次刷新，都去中央底座拿最新的状态
    m_mockDatabase = getCentralMockDatabase();
    endResetModel();
}

int FaceDatabaseViewModel::rowCount(const QModelIndex &) const { return m_mockDatabase.size(); }
int FaceDatabaseViewModel::columnCount(const QModelIndex &) const { return 5; }

QVariant FaceDatabaseViewModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) return QVariant();

    const auto& user = m_mockDatabase[index.row()];
    switch (index.column()) {
    case 0: return user["id"];
    case 1: return user["name"];
    case 2: return user["workId"];
    case 3: return user["privilege"];
    case 4: return user["time"];
    }
    return QVariant();
}

QVariant FaceDatabaseViewModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) return QVariant();
    QStringList headers = {"凭证ID", "姓名", "工号/卡号", "通行权限", "录入时间"};
    return headers[section];
}

void FaceDatabaseViewModel::removeUser(int row) {
    if (row < 0 || row >= m_mockDatabase.size()) return;

    beginRemoveRows(QModelIndex(), row, row);

    // 【★核心修改】同步把中央全局底座里的这个人也删掉！
    auto& centralDb = getCentralMockDatabase();
    QString deletedName = m_mockDatabase[row]["name"].toString();

    // 根据名字从中央底座彻底抹去
    for(int i = 0; i < centralDb.size(); ++i) {
        if(centralDb[i]["name"].toString() == deletedName) {
            centralDb.removeAt(i);
            break;
        }
    }

    m_mockDatabase.removeAt(row); // 刷新当前 UI 表格
    endRemoveRows();
}

// 供外部智能分析模块调用的全局查询接口
bool FaceDatabaseViewModel::isUserAllowed(const QString& name, QString& outReason) {
    auto& centralDb = getCentralMockDatabase();
    for (const auto& user : centralDb) {
        if (user["name"].toString() == name) {
            if (user["privilege"].toString() == "允许通行") {
                return true;
            } else {
                outReason = "该用户权限已被修改为【禁止通行】！";
                return false;
            }
        }
    }
    outReason = "系统白名单库中无此人（凭证已被注销）！";
    return false;
}