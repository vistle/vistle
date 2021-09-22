#ifndef LOCALFILESYSTEMMODEL_H
#define LOCALFILESYSTEMMODEL_H

#include "abstractfilesystemmodel.h"
#include <QAbstractProxyModel>

class FSModel;

class LocalFileSystemModel: public AbstractFileSystemModel //, public QAbstractProxyModel
{
    Q_OBJECT

public:
    LocalFileSystemModel(QObject *parent = nullptr);
    ~LocalFileSystemModel() override;

    QModelIndex buddy(const QModelIndex &idx) const override;

#if 1
    // AbstractFileSystemModel
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(const QString &path, int column = 0) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    using QObject::parent;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant myComputer(int role = Qt::DisplayRole) const override;
    QVariant homePath(int role = Qt::DisplayRole) const override;
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
#endif

    bool isRootDir(const QString &path) const override;
    QString workingDirectory() const override;

    // QFileSystemModel specific API
    QModelIndex setRootPath(const QString &path) override;
    QString rootPath() const override;
    QDir rootDirectory() const;

    void setIconProvider(RemoteFileIconProvider *provider) override;
    RemoteFileIconProvider *iconProvider() const override;

    void setFilter(QDir::Filters filters) override;
    QDir::Filters filter() const override;

    void setResolveSymlinks(bool enable) override;
    bool resolveSymlinks() const override;

    void setDisableRecursiveSort(bool enable) override;
    bool disableRecursiveSort() const override;

    void setReadOnly(bool enable) override;
    bool isReadOnly() const override;

    void setNameFilterDisables(bool enable) override;
    bool nameFilterDisables() const override;

    void setNameFilters(const QStringList &filters) override;
    QStringList nameFilters() const override;

    QString filePath(const QModelIndex &index) const override;
    bool isDir(const QModelIndex &index) const override;
    qint64 size(const QModelIndex &index) const override;
    QString type(const QModelIndex &index) const override;
    QDateTime lastModified(const QModelIndex &index) const override;

    QModelIndex mkdir(const QModelIndex &parent, const QString &name) override;
    inline QString fileName(const QModelIndex &index) const override;
    inline QIcon fileIcon(const QModelIndex &index) const override;
    QFile::Permissions permissions(const QModelIndex &index) const override;
    FileInfo fileInfo(const QModelIndex &index) const override;
    bool rmdir(const QModelIndex &index);
    bool remove(const QModelIndex &index);

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;


protected:
#if 0
    void timerEvent(QTimerEvent *event) override;
    bool event(QEvent *event) override;
#endif

private:
    QScopedPointer<FSModel> m_model;
    QList<QPersistentModelIndex> layoutChangePersistentIndexes;
    QModelIndexList proxyIndexes;
    QScopedPointer<RemoteFileIconProvider> m_fileIconProvider;

    Q_DISABLE_COPY(LocalFileSystemModel)
};

#endif
