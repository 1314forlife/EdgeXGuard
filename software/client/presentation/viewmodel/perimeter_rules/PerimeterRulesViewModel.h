#ifndef PERIMETER_RULES_VIEWMODEL_H
#define PERIMETER_RULES_VIEWMODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include <QVariantList>

class PerimeterRulesViewModel : public QAbstractTableModel {
    Q_OBJECT
public:
    static PerimeterRulesViewModel& instance();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

public slots:
    void loadMockRules();
    void addRule(const QString& name, const QString& trigger, const QString& action);
    void removeRule(int row);
    void toggleRuleStatus(int row); // 切换规则启用/禁用状态

private:
    explicit PerimeterRulesViewModel(QObject* parent = nullptr);
    ~PerimeterRulesViewModel() = default;

    QList<QVariantMap> m_mockRules;
};

#endif // PERIMETER_RULES_VIEWMODEL_H