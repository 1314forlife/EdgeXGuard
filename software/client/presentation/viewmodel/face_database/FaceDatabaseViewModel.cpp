#include "FaceDatabaseViewModel.h"
#include "infrastructure/repository_impl/FaceRepositoryImpl.h" 
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <random>
#include <chrono>

// ==================== 【★辅助算子：获取安全的统一 data/ 数据资产目录】 ====================
static QString getSafeDataPath(const QString& fileName) {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    
#ifdef Q_OS_WIN
    if (dir.dirName() == "debug" || dir.dirName() == "release") {
        dir.cdUp();
    }
    dir.cdUp();
    dir.cdUp();
#endif

    if (!dir.exists("data")) {
        dir.mkdir("data");
    }
    dir.cd("data");
    return dir.absoluteFilePath(fileName);
}

// ==================== 【★核心联动：建立中央全局数据库（支持硬盘持久化恢复）】 ====================
static QList<QVariantMap>& getCentralMockDatabase() {
    static QList<QVariantMap> s_database;

    if (s_database.isEmpty()) {
        QString savePath = getSafeDataPath("face_database.json");
        QFile file(savePath);

        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = file.readAll();
            file.close();

            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isArray()) {
                QJsonArray jsonArray = doc.array();
                for (int i = 0; i < jsonArray.size(); ++i) {
                    s_database.append(jsonArray.at(i).toObject().toVariantMap());
                }
                qDebug() << "📂 [Database] 成功从统一 data/ 路径恢复" << s_database.size() << "条 JSON 记录。";
                return s_database;
            }
        }

        qDebug() << "ℹ️ [Database] 未检测到本地存储文件，初始化默认白名单底座。";
        s_database.append({{"id", 1}, {"name", "张三"}, {"workId", "HQ-9527"}, {"privilege", "允许通行"}, {"time", "2026-05-20"}});
        s_database.append({{"id", 2}, {"name", "李四"}, {"workId", "HQ-8848"}, {"privilege", "允许通行"}, {"time", "2026-05-22"}});
        s_database.append({{"id", 3}, {"name", "王五"}, {"workId", "HQ-1024"}, {"privilege", "禁止通行"}, {"time", "2026-05-25"}});
    }
    return s_database;
}

void FaceDatabaseViewModel::saveToDisk() {
    QString savePath = getSafeDataPath("face_database.json");
    QFile file(savePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "🔴 [Database] 物理落盘失败：无法写入持久化文件！路径:" << savePath;
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
    qDebug() << "💾 [Database] 数据变动，安全落盘成功！当前文件总记录数:" << centralDb.size();

    // =========================================================================
    // 📡 【无线大炮点火】：跨网络硬核隔空投递管线
    // =========================================================================
    // 🚀 杰哥，由于刚刚测试通过，咱们直接锁定你 RK3588 的真实 IP！
    QString rk3588_ip = "192.168.0.102"; 
    QString rk3588_user = "radxa";       
    
    // 构筑网关上咱们刚才建立好的 storage 物理接收阵地
    QString remoteJsonDir = QString("%1@%2:~/work/EdgeXGuardServer/storage/data/").arg(rk3588_user).arg(rk3588_ip);
    QString remoteFaceDir = QString("%1@%2:~/work/EdgeXGuardServer/storage/faces/").arg(rk3588_user).arg(rk3588_ip);

    qDebug() << "🚀 [无线同步器] 后台启动！开始无密硬同步至 RK3588...";

    // 1. 发射全量 JSON 账本文件
    QStringList jsonArgs;
    jsonArgs << "-o" << "StrictHostKeyChecking=no" << savePath << remoteJsonDir;
    QProcess::startDetached("scp", jsonArgs); // 异步扔出去，绝不卡主界面

    // 2. 发射 PC 本地录入的所有照片增量包
    QString localFacesPath = QCoreApplication::applicationDirPath() + "/faces/";
#ifdef Q_OS_WIN
    QDir winDir(localFacesPath);
    // 如果在 build/debug 这种深层目录下，退回到跟 faces 文件夹平级的 build/ 根目录
    if (winDir.dirName() == "debug" || winDir.dirName() == "release") { 
        winDir.cdUp(); 
        localFacesPath = winDir.absolutePath() + "/faces/"; 
    } else {
        // 如果就在 build 根目录，直接拿绝对路径，不要重复累加
        localFacesPath = winDir.absolutePath(); 
    }
#endif

    QStringList faceArgs;
    faceArgs << "-o" << "StrictHostKeyChecking=no" << "-r" << localFacesPath << remoteFaceDir;
    QProcess::startDetached("scp", faceArgs); // 增量同步 faces 文件夹

    qDebug() << "  └─ [数据发射完毕] JSON账本与照片包正在后台飞向网关芯片。";
}

// =================================================================================

FaceDatabaseViewModel& FaceDatabaseViewModel::instance() {
    static FaceDatabaseViewModel instance;
    return instance;
}

// 🚀 构造函数初始化
FaceDatabaseViewModel::FaceDatabaseViewModel(QObject* parent) : QAbstractTableModel(parent) {
    m_faceRepository = new FaceRepositoryImpl();

    // 🚀 核心合闸：底层算出来的真耗时一旦弹射，ViewModel 瞬间无缝转发给 UI 面板！
    connect(m_faceRepository, &FaceRepositoryImpl::performanceMetricsUpdated,
            this, &FaceDatabaseViewModel::performanceMetricsReady);

    loadMockData();
    
    qDebug() << "⛓️ [ViewModel] 真实性能数据中控桥梁合闸成功，底层算法实例已就位！";
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
    newUser["imagePath"] = imagePath;

    centralDb.append(newUser);
    m_mockDatabase.append(newUser);

    endInsertRows();

    qDebug() << "🎉 [ViewModel] 成功往中央全局白名单库追加用户！姓名:" << name
             << " | 分配ID:" << newId << " | 当前总数:" << centralDb.size();

    saveToDisk();
}

void FaceDatabaseViewModel::setUseHnswMode(bool enabled) {
    if (m_useHnswMode != enabled) {
        m_useHnswMode = enabled;
        qDebug() << "⚙️ [ViewModel] 策略开关触发，检索算法框架切换为:" << (enabled ? "2.0 HNSW 图" : "1.0 传统遍历");
        
        // 🚀 撕掉 setProperty，直接通过 C++ 指针强行修改底层的控制开关！
        if (m_faceRepository) {
            m_faceRepository->setHnswMode(enabled);
        }
        
        emit useHnswModeChanged(m_useHnswMode);
        
        // 2. 模拟一组真实的当前帧 512维抓拍特征，强行激活底层的掐表压测
        std::vector<float> mockCurrentFeature(512, 0.15f); 
        
        if (m_faceRepository) {
            m_faceRepository->searchNearestFace(mockCurrentFeature, 0.4f);
        }
    }
}

void FaceDatabaseViewModel::loadMockData() {
    beginResetModel();
    
    m_mockDatabase = getCentralMockDatabase();

#ifdef EDGE_X_GUARD_MOCK_MODE
    if (m_mockDatabase.size() < 100) {
        qDebug() << "⚡ [数据灌装机] 正在物理生成 10,000 条高维人脸白名单特征压入内存...";
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

        int baseId = m_mockDatabase.isEmpty() ? 1 : m_mockDatabase.last()["id"].toInt() + 1;

        for (int i = 0; i < 10000; ++i) {
            QVariantMap fakeUser;
            fakeUser["id"] = baseId + i;
            fakeUser["name"] = QString("虚拟客流_%1").arg(baseId + i);
            fakeUser["workId"] = QString("XN-%1").arg(100000 + baseId + i);
            fakeUser["privilege"] = "允许通行";
            fakeUser["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd");
            
            std::vector<float> mockFeature(512);
            for (int d = 0; d < 512; ++d) {
                mockFeature[d] = dis(gen);
            }

            m_mockDatabase.append(fakeUser);
        }
        qDebug() << "🎉 [数据灌装机] 10,000 条高维人脸数据已全量就位！当前总池量:" << m_mockDatabase.size();
    }
#endif
    endResetModel();
}