#include "FaceRepositoryImpl.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

FaceRepositoryImpl::FaceRepositoryImpl() {
    initializeDatabase(); // 🟢 开机初始化数据库
}

void FaceRepositoryImpl::initializeDatabase() {
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        return;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("edgexguard.db");

    if (!db.open()) {
        qDebug() << "❌ [DB] SQLite 物理开机失败:" << db.lastError().text();
        return;
    }
    qDebug() << "💾 [DB] 成功连接/创建本地物理数据库 (build/edgexguard.db)";

    QSqlQuery query;
    // 🟢 字段瘦身：只建立实体类里确实存在的核心字段
    query.exec("CREATE TABLE IF NOT EXISTS face_whitelist ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT, "
               "work_id TEXT UNIQUE)");
}

bool FaceRepositoryImpl::addFace(const FaceEntity& face) {
    QSqlQuery query;

    // 🟢 严格对接你的结构体：只绑定 name 和 workId
    query.prepare("INSERT OR REPLACE INTO face_whitelist (name, work_id) VALUES (:name, :work_id)");
    query.bindValue(":name", face.name);
    query.bindValue(":work_id", face.workId); // 👈 严格匹配你的结构体变量名

    if (!query.exec()) {
        qWarning() << "❌ [DB] 物理压盘落库失败！工号:" << face.workId << "报错:" << query.lastError().text();
        return false;
    }

    qDebug() << "🎉 [DB] 物理压盘落库大获全胜！姓名:" << face.name << "工号:" << face.workId;
    return true;
}

std::optional<FaceEntity> FaceRepositoryImpl::searchNearestFace(const std::vector<float>& faceFeature, float threshold) {
    Q_UNUSED(faceFeature);
    Q_UNUSED(threshold);
    return std::nullopt;
}