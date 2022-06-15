#ifndef ABSTRACTFILESYSTEMMODEL_H
#define ABSTRACTFILESYSTEMMODEL_H

#include <QFileSystemModel>

#include "abstractfileinfogatherer.h"

class AbstractFileSystemModel: public QAbstractItemModel {
    Q_OBJECT
    Q_PROPERTY(bool resolveSymlinks READ resolveSymlinks WRITE setResolveSymlinks)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool nameFilterDisables READ nameFilterDisables WRITE setNameFilterDisables)

Q_SIGNALS:
    void rootPathChanged(const QString &newPath);
    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void directoryLoaded(const QString &path);

public:
    enum Roles {
        FileIconRole = Qt::DecorationRole,
        FilePathRole = Qt::UserRole + 1,
        FileNameRole = Qt::UserRole + 2,
        FilePermissions = Qt::UserRole + 3
    };

    AbstractFileSystemModel(QObject *parent = nullptr);
    ~AbstractFileSystemModel() override;

    QHash<int, QByteArray> roleNames() const override;

    virtual QString identifier() const = 0;

    // QFileSystemModel
#if 0
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
#endif
    virtual QModelIndex fsIndex(const QString &path, int column = 0) const = 0;
#if 0
    QModelIndex parent(const QModelIndex &child) const override;
    using QObject::parent;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
#endif

    virtual QVariant myComputer(int role = Qt::DisplayRole) const = 0;
    virtual QVariant homePath(int role = Qt::DisplayRole) const = 0;
    virtual QString userName() const = 0;

#if 0
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;
#endif

    virtual bool isRootDir(const QString &path) const = 0;
    virtual QString workingDirectory() const = 0;
    virtual bool isWindows() const = 0;

    // QFileSystemModel specific API
    virtual QModelIndex setRootPath(const QString &path) = 0;
    virtual QString rootPath() const = 0;

    virtual void setIconProvider(RemoteFileIconProvider *provider) = 0;
    virtual RemoteFileIconProvider *iconProvider() const = 0;

    virtual void setFilter(QDir::Filters filters) = 0;
    virtual QDir::Filters filter() const = 0;

    virtual void setResolveSymlinks(bool enable) = 0;
    virtual bool resolveSymlinks() const = 0;

    virtual void setDisableRecursiveSort(bool enable) = 0;
    virtual bool disableRecursiveSort() const = 0;

    virtual void setReadOnly(bool enable) = 0;
    virtual bool isReadOnly() const = 0;

    virtual void setNameFilterDisables(bool enable) = 0;
    virtual bool nameFilterDisables() const = 0;

    virtual void setNameFilters(const QStringList &filters) = 0;
    virtual QStringList nameFilters() const = 0;

    virtual QString filePath(const QModelIndex &index) const = 0;
    virtual bool isDir(const QModelIndex &index) const = 0;
    virtual qint64 size(const QModelIndex &index) const = 0;
    virtual QString type(const QModelIndex &index) const = 0;
    virtual QDateTime lastModified(const QModelIndex &index) const = 0;

    virtual QModelIndex mkdir(const QModelIndex &parent, const QString &name) = 0;
    virtual inline QString fileName(const QModelIndex &index) const = 0;
    virtual inline QIcon fileIcon(const QModelIndex &index) const = 0;
    virtual QFile::Permissions permissions(const QModelIndex &index) const = 0;
    virtual FileInfo fileInfo(const QModelIndex &index) const = 0;
#ifdef REMOVE
    virtual bool rmdir(const QModelIndex &index) = 0;
    virtual bool remove(const QModelIndex &index) = 0;
#endif

protected:
#if 0
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
    bool event(QEvent *event) Q_DECL_OVERRIDE;
#endif

private:
    Q_DISABLE_COPY(AbstractFileSystemModel)

#if 0
    Q_PRIVATE_SLOT(d_func(), void _q_directoryChanged(const QString &directory, const QStringList &list))
    Q_PRIVATE_SLOT(d_func(), void _q_performDelayedSort())
    Q_PRIVATE_SLOT(d_func(), void _q_fileSystemChanged(const QString &path, const QVector<QPair<QString, FileInfo> > &))
    Q_PRIVATE_SLOT(d_func(), void _q_resolvedName(const QString &fileName, const QString &resolvedName))
#endif
};

#endif
