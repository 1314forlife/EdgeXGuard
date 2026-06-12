#include "FaceRepositoryImpl.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <cmath>
#include <chrono>
#include "hnswlib.h" 

FaceRepositoryImpl::FaceRepositoryImpl() {
    initializeDatabase(); // 初始化 SQLite

    // 🚀 2. 在板载/客户端内存中初始化 HNSW 拓扑检索空间
    try {
        // 人脸特征通常是 512 维
        int dimensions = 512; 
        // 预设最大支持 20000 条人脸数据
        int maxElements = 20000; 

        // 实例化内积空间 (InnerProductSpace)
        m_hnswSpace = new hnswlib::InnerProductSpace(dimensions);
        
        // 实例化 HNSW 核心算法类
        // 参数含义: 空间指针, 最大元素量, M(节点连接数=16), efConstruction(构建优化参数=200)
        m_hnswIndex = new hnswlib::HierarchicalNSW<float>(m_hnswSpace, maxElements, 16, 200);
        
        qDebug() << "💾 [HNSW] 内存高维向量拓扑图检索空间初始化成功";
    } catch (const std::exception& e) {
        qWarning() << "❌ [HNSW] 空间初始化失败:" << e.what();
    }
}

FaceRepositoryImpl::~FaceRepositoryImpl() {
    // 🚀 3. 析构时优雅释放 C++ 裸指针，防止内存泄漏
    if (m_hnswIndex) { delete m_hnswIndex; m_hnswIndex = nullptr; }
    if (m_hnswSpace) { delete m_hnswSpace; m_hnswSpace = nullptr; }
}

void FaceRepositoryImpl::initializeDatabase() {
    // 1. 防御性断言：如果默认连接已经存在，直接返回，避免重复挂载
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        return;
    }

    // 2. 动态获取当前可执行文件所在的物理构建路径
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    
    // 3. 跨平台路径回溯算子：跳出临时构建大本营（build/），回退到源码根目录
#ifdef Q_OS_WIN
    if (dir.dirName() == "debug" || dir.dirName() == "release") {
        dir.cdUp(); // 跳出 debug/release 目录
    }
    dir.cdUp(); // 跳出目标编译器临时夹
    dir.cdUp(); // 跳出 build 大本营
#endif

    // 4. 核心物理防护：如果检测到源码根目录下没有 data 文件夹，代码执行增量强行创建
    if (!dir.exists("data")) {
        dir.mkdir("data");
    }
    dir.cd("data"); // 进入受控的 data 数据存储大本营

    // 5. 锁定最终的本地关系型数据库物理全路径
    QString absoluteDbPath = dir.absoluteFilePath("edgexguard.db");
    
    // 6. 挂载物理驱动并执行开机握手
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(absoluteDbPath);

    if (!db.open()) {
        qDebug() << "❌ [DB] SQLite 离线数据库安全开机失败:" << db.lastError().text();
        return;
    }
    
    // 🟢 成功提示：运行时会在控制台打印出锁死的绝对路径，方便你随时进文件夹查看
    qDebug() << "💾 [DB] 数据库资产已成功合闸并死锁在安全存储区:" << absoluteDbPath;

    // 7. 执行白名单表结构初始化
    QSqlQuery query;
    // 增加 feature_blob 字段用于存入后续人脸识别所需的 512 维高维浮点特征向量原料
    query.exec("CREATE TABLE IF NOT EXISTS face_whitelist ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT, "
               "work_id TEXT UNIQUE, "
               "feature_blob BLOB)"); 
}

bool FaceRepositoryImpl::addFace(const FaceEntity& face) {
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO face_whitelist (name, work_id, feature_blob) VALUES (:name, :work_id, :feature_blob)");
    query.bindValue(":name", face.name);
    query.bindValue(":work_id", face.workId);
    
    // 动态将 std::vector<float> 转为 QByteArray 存入数据库 BLOB 字段
    QByteArray featureData(reinterpret_cast<const char*>(face.feature.data()), face.feature.size() * sizeof(float));
    query.bindValue(":feature_blob", featureData);

    if (!query.exec()) {
        qWarning() << "❌ [DB] 压盘落库失败:" << query.lastError().text();
        return false;
    }

    // 🚀 核心动作：当新录入人脸时，同步将其特征向量塞入 HNSW 内存图谱中
    // 这里的 id 必须是整数作为 HNSW 的 Label，这里暂时用自增 rowid 或工号 hash
    hnswlib::labeltype label = std::hash<std::string>{}(face.workId.toStdString());
    m_hnswIndex->addPoint(face.feature.data(), label);

    qDebug() << "🎉 [DB+HNSW] 数据双写成功！姓名:" << face.name;
    return true;
}

// 🎯 核心功能：根据开关动态切换检索策略
std::optional<FaceEntity> FaceRepositoryImpl::searchNearestFace(const std::vector<float>& faceFeature, float threshold) {
    
    // 这里的 m_useHnswMode 开关后续通过 ViewModel 动态控制
    if (!m_useHnswMode) {
        // ---------------------------------------------------------
        // 【传统模式】：1.0 暴力遍历扫表 (O(N) 时间复杂度)
        // ---------------------------------------------------------
        auto start = std::chrono::high_resolution_clock::now();
        
        QSqlQuery query("SELECT name, work_id, feature_blob FROM face_whitelist");
        float max_similarity = -1.0f;
        FaceEntity best_match;
        bool found = false;

        while (query.next()) {
            QString name = query.value(0).toString();
            QString workId = query.value(1).toString();
            QByteArray blob = query.value(2).toByteArray();
            
            std::vector<float> saved_feature(blob.size() / sizeof(float));
            std::memcpy(saved_feature.data(), blob.constData(), blob.size());

            // 手动计算余弦相似度
            float dot_product = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
            for (size_t i = 0; i < faceFeature.size() && i < saved_feature.size(); ++i) {
                dot_product += faceFeature[i] * saved_feature[i];
                norm_a += faceFeature[i] * faceFeature[i];
                norm_b += saved_feature[i] * saved_feature[i];
            }
            float similarity = (norm_a == 0 || norm_b == 0) ? 0.0f : (dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b)));

            if (similarity > max_similarity && similarity >= threshold) {
                max_similarity = similarity;
                best_match.name = name;
                best_match.workId = workId;
                found = true;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        qDebug() << "⏱️ [传统遍历模式] 耗时:" << elapsed_ms << "ms";
        
        // 触发耗时统计回调更新（后续对接 ViewModel 弹射给 UI）
        emit performanceMetricsUpdated(elapsed_ms * 1000.0, "BruteForce"); 

        if (found) return best_match;
        
    } else {
        // ---------------------------------------------------------
        // 【高级模式】：2.0 HNSW 图拓扑快速检索 (O(log N) 时间复杂度)
        // ---------------------------------------------------------
        auto start = std::chrono::high_resolution_clock::now();
        
        // 执行 K 近邻查询 (K=1，找出最相似的 1 个人)
        std::priority_queue<std::pair<float, hnswlib::labeltype>> result = 
            m_hnswIndex->searchKnn(faceFeature.data(), 1);
        
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_us = std::chrono::duration<double, std::micro>(end - start).count();
        qDebug() << "⏱️ [HNSW 图检索模式] 耗时:" << elapsed_us << "us";
        
        // 发送微秒级耗时给 UI 看板
        emit performanceMetricsUpdated(elapsed_us, "HNSW");

        if (!result.empty()) {
            auto top_match = result.top();
            float distance = top_match.first; // 内积距离
            hnswlib::labeltype matched_label = top_match.second;

            // 根据距离反推相似度并匹配 SQLite 数据库中的实体信息
            // 工业落地实现：根据 label(hash) 去查对应的 name 和 workId 并返回
            if (distance < (1.0f - threshold)) { 
               // 找到对应的实体并返回...
            }
        }
    }

    return std::nullopt;
}