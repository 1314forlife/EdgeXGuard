#ifndef FACE_REPOSITORY_IMPL_H
#define FACE_REPOSITORY_IMPL_H

#include "domain/repository/IFaceRepository.h"
#include <QObject>
#include <vector>
#include <optional>
#include <QString>

// 🚀 前向声明 hnswlib 的核心类，防止头文件相互嵌套污染
namespace hnswlib {
    class InnerProductSpace;
    template<typename MPTYPE> class HierarchicalNSW;
}

// 🚀 继承 QObject 以便使用 Qt 的信号槽（Signals & Slots）机制
class FaceRepositoryImpl : public QObject, public IFaceRepository {
    Q_OBJECT

public:
    FaceRepositoryImpl(); 
    virtual ~FaceRepositoryImpl(); // 🟢 改为普通虚析构，在 .cpp 里手动释放 HNSW 物理指针

    bool addFace(const FaceEntity& face) override;
    std::optional<FaceEntity> searchNearestFace(const std::vector<float>& faceFeature, float threshold) override;

    // 🎯 留给 ViewModel 层调用的控制策略开关
    void setHnswMode(bool enabled) { m_useHnswMode = enabled; }
    bool isHnswMode() const { return m_useHnswMode; }

signals:
    // 🎯 核心信号：底层计算完耗时后，直接弹射给 ViewModel，最终通知 UI 看板更新
    // timeCountUs: 耗时（微秒），modeName: "BruteForce" 或 "HNSW"
    void performanceMetricsUpdated(double timeCountUs, QString modeName);

private:
    void initializeDatabase();

private:
    bool m_useHnswMode = false; // 🚀 1.0 遍历模式与 2.0 HNSW 模式的切换中控开关

    // 🧠 HNSW 内存图谱检索组件指针
    hnswlib::InnerProductSpace* m_hnswSpace = nullptr;
    hnswlib::HierarchicalNSW<float>* m_hnswIndex = nullptr;

    const int m_dimensions = 512;     // 锁定人脸特征为标准的 512 维
    const int m_maxElements = 20000;  // 内存拓扑图最大支持的人脸样本容量
};

#endif // FACE_REPOSITORY_IMPL_H