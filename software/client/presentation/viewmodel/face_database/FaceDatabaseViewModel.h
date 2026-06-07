#ifndef FACE_DATABASE_VIEWMODEL_H
#define FACE_DATABASE_VIEWMODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include <QVariantList>

class FaceDatabaseViewModel : public QAbstractTableModel {
    Q_OBJECT
public:
    static FaceDatabaseViewModel& instance();

    // 标准的 QAbstractTableModel 虚函数实现，用来给 QTableView 喂数据
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    static bool isUserAllowed(const QString& name, QString& outReason);

    void saveToDisk();

    // 🟢 【★核心追加】在这里声明我们的内存插值接口，允许外部弹窗/主页调用它
    void addUser(const QString& name, const QString& workId, const QString& imagePath = "");

public slots:
    void loadMockData();      // 加载我们的假数据库数据
    void removeUser(int row); // 模拟删除用户

private:
    explicit FaceDatabaseViewModel(QObject* parent = nullptr);
    ~FaceDatabaseViewModel() = default;

    QList<QVariantMap> m_mockDatabase; // 我们的“内存假数据库”
};

#endif // FACE_DATABASE_VIEWMODEL_H