#ifndef VTREEMODEL_H
#define VTREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#if 0

class TreeItem;

class VTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    VTreeModel(const QString &data, QObject *parent = 0);
    ~VTreeModel();

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    void setupModelData(const QStringList &lines, VTreeItem *parent);
    VTreeItem *rootItem;
};

#endif // VTREEMODEL_H

#endif
