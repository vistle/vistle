#ifndef VISTLE_FILESYSTEM_MODEL_H
#define VISTLE_FILESYSTEM_MODEL_H

#include <QFileSystemModel>

class VistleFileSystemModel: public QFileSystemModel {
    Q_OBJECT

public:
    VistleFileSystemModel(QObject *parent = nullptr);
    ~VistleFileSystemModel() override;

    // Vistle
    void setHub(int hubId);
    int hub() const;

    // QFileSystemModel
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(const QString &path, int column = 0) const;
    QModelIndex parent(const QModelIndex &child) const override;
    using QObject::parent;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant myComputer(int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                      const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;

private:
    int m_hub = 0;
};

#endif
