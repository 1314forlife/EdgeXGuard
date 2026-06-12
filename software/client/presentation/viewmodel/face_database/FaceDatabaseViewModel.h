#ifndef FACE_DATABASE_VIEWMODEL_H
#define FACE_DATABASE_VIEWMODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include <QVariantList>
#include <QProcess>
//#define EDGE_X_GUARD_MOCK_MODE

// 前置声明底层仓储类，彻底斩断头文件交叉包含的死锁
class FaceRepositoryImpl; 

class FaceDatabaseViewModel : public QAbstractTableModel {
    Q_OBJECT
    
    // 🚀 【核心】挂载 Qt 属性系统，MOC 编译器全靠这一行来生成反射代码
    Q_PROPERTY(bool useHnswMode READ useHnswMode WRITE setUseHnswMode NOTIFY useHnswModeChanged)

public:
    static FaceDatabaseViewModel& instance();

    // 🚀 属性系统配套的读函数（Gettter）
    bool useHnswMode() const { return m_useHnswMode; }
    
    // 🚀 属性系统配套的写函数（Setter）- 已经在 .cpp 中实现
    void setUseHnswMode(bool enabled);

    // 标准 QAbstractTableModel 虚函数，负责给前端 QTableView 喂万级数据
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    static bool isUserAllowed(const QString& name, QString& outReason);
    void saveToDisk();
    void addUser(const QString& name, const QString& workId, const QString& imagePath = "");

public slots:
    void loadMockData();      // 内部挂载了万级高维向量随机数据灌装机
    void removeUser(int row); // 注销人员

signals:
    // 🚀 MOC 报错说缺少的信号，在这里全量归位！
    void useHnswModeChanged(bool enabled);
    
    // 🚀 负责把底层 std::chrono 掐出来的真耗时（微秒级）瞬间弹射给前端 UI 看板
    void performanceMetricsReady(double timeCountUs, QString modeName);

private:
    explicit FaceDatabaseViewModel(QObject* parent = nullptr);
    virtual ~FaceDatabaseViewModel() = default;

    QList<QVariantMap> m_mockDatabase; // 内存中央白名单底座矩阵
    
    bool m_useHnswMode = false;        // 常驻内存的算法策略开关状态
    FaceRepositoryImpl* m_faceRepository = nullptr; // 底层关系型数据库与 HNSW 图空间大管家指针
};

#endif // FACE_DATABASE_VIEWMODEL_H