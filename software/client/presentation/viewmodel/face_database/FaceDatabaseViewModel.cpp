#include "FaceDatabaseViewModel.h"
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

// ==================== 【★核心联动：建立中央全局数据库（支持硬盘持久化恢复）】 ====================
static QList<QVariantMap>& getCentralMockDatabase() {
    static QList<QVariantMap> s_database;

    if (s_database.isEmpty()) {
        QString savePath = "face_database.json";
        QFile file(savePath);

        // 🟢 优先策略：如果本地硬盘存在保存好的数据文件，直接从硬盘恢复数据！
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = file.readAll();
            file.close();

            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isArray()) {
                QJsonArray jsonArray = doc.array();
                for (int i = 0; i < jsonArray.size(); ++i) {
                    s_database.append(jsonArray.at(i).toObject().toVariantMap());
                }
                qDebug() << "📂 [Database] 检测到本地持久化文件，成功从硬盘恢复" << s_database.size() << "条记录。";
                return s_database;
            }
        }

        // 🟡 保底策略：如果硬盘没有文件（第一次开机），则使用原本的 3 条初始底座数据
        qDebug() << "ℹ️ [Database] 未检测到本地存储文件，初始化默认白名单底座。";
        s_database.append({{"id", 1}, {"name", "张三"}, {"workId", "HQ-9527"}, {"privilege", "允许通行"}, {"time", "2026-05-20"}});
        s_database.append({{"id", 2}, {"name", "李四"}, {"workId", "HQ-8848"}, {"privilege", "允许通行"}, {"time", "2026-05-22"}});
        s_database.append({{"id", 3}, {"name", "王五"}, {"workId", "HQ-1024"}, {"privilege", "禁止通行"}, {"time", "2026-05-25"}});
    }
    return s_database;
}

// 🟢 新增：统一落盘逻辑（将内存中的中央底座矩阵序列化为本地 JSON 文件）
void FaceDatabaseViewModel::saveToDisk() {
    QString savePath = "face_database.json";
    QFile file(savePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "🔴 [Database] 物理落盘失败：无法写入持久化文件！";
        return;
    }

    auto& centralDb = getCentralMockDatabase();
    QJsonArray jsonArray;
    for (const auto& user : centralDb) {
        jsonArray.append(QJsonObject::fromVariantMap(user));
    }

    QJsonDocument doc(jsonArray);
    file.write(doc.toJson());
    file.close();
    qDebug() << "💾 [Database] 数据变动，物理落盘成功！当前文件总记录数:" << centralDb.size();
}
// =================================================================================

FaceDatabaseViewModel& FaceDatabaseViewModel::instance() {
    static FaceDatabaseViewModel instance;
    return instance;
}

FaceDatabaseViewModel::FaceDatabaseViewModel(QObject* parent) : QAbstractTableModel(parent) {
    loadMockData();
}

void FaceDatabaseViewModel::loadMockData() {
    beginResetModel();
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

    auto& centralDb = getCentralMockDatabase();
    QString deletedName = m_mockDatabase[row]["name"].toString();

    for(int i = 0; i < centralDb.size(); ++i) {
        if(centralDb[i]["name"].toString() == deletedName) {
            centralDb.removeAt(i);
            break;
        }
    }

    m_mockDatabase.removeAt(row);
    endRemoveRows();

    // 🟢 核心改动：删除用户成功后，立刻同步落盘！
    saveToDisk();
}

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

void FaceDatabaseViewModel::addUser(const QString& name, const QString& workId, const QString& imagePath) {
    Q_UNUSED(imagePath);

    auto& centralDb = getCentralMockDatabase();
    int newId = centralDb.isEmpty() ? 1 : centralDb.last()["id"].toInt() + 1;
    int insertRow = m_mockDatabase.size();

    beginInsertRows(QModelIndex(), insertRow, insertRow);

    QVariantMap newUser;
    newUser["id"] = newId;
    newUser["name"] = name;
    newUser["workId"] = workId;
    newUser["privilege"] = "允许通行";
    newUser["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    centralDb.append(newUser);
    m_mockDatabase.append(newUser);

    endInsertRows();

    qDebug() << "🎉 [ViewModel] 成功往中央全局白名单库追加用户！姓名:" << name
             << " | 分配ID:" << newId << " | 当前总数:" << centralDb.size();

    // 🟢 核心改动：新增用户成功后，立刻同步落盘！
    saveToDisk();
}