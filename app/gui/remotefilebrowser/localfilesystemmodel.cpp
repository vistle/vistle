#include "localfilesystemmodel.h"
#include <QDateTime>
#include <QApplication>
#include <QStyle>

// to make createIndex accessible
class FSModel: public QFileSystemModel {
    friend class LocalFileSystemModel;

    FSModel(QObject *parent): QFileSystemModel(parent) {}
    using QFileSystemModel::createIndex;
};

LocalFileSystemModel::LocalFileSystemModel(QObject *parent)
: AbstractFileSystemModel(parent), m_model(new FSModel(this)), m_fileIconProvider(new RemoteFileIconProvider)
{
    connect(&*m_model, &FSModel::dataChanged,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
                emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), roles);
            });
    connect(&*m_model, &FSModel::rootPathChanged, [this](const QString &path) { emit rootPathChanged(path); });
    connect(&*m_model, &FSModel::fileRenamed,
            [this](const QString &path, const QString &oldName, const QString &newName) {
                emit fileRenamed(path, oldName, newName);
            });
    connect(&*m_model, &FSModel::directoryLoaded, [this](const QString &path) {
        qInfo() << "LocalFileSystemModel: directory loaded:" << path;
        emit directoryLoaded(path);
    });

    connect(&*m_model, &FSModel::headerDataChanged, [this](Qt::Orientation orientation, int first, int last) {
        emit headerDataChanged(orientation, first, last);
    });

    connect(&*m_model, &FSModel::layoutAboutToBeChanged,
            [this](const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint) {
                QList<QPersistentModelIndex> parents;
                parents.reserve(sourceParents.size());
                for (const QPersistentModelIndex &parent: sourceParents) {
                    if (!parent.isValid()) {
                        parents << QPersistentModelIndex();
                        continue;
                    }
                    const QModelIndex mappedParent = mapFromSource(parent);
                    Q_ASSERT(mappedParent.isValid());
                    parents << mappedParent;
                }

                emit layoutAboutToBeChanged(parents, hint);

                const auto proxyPersistentIndexes = persistentIndexList();
                for (const QPersistentModelIndex &proxyPersistentIndex: proxyPersistentIndexes) {
                    proxyIndexes << proxyPersistentIndex;
                    Q_ASSERT(proxyPersistentIndex.isValid());
                    const QPersistentModelIndex srcPersistentIndex = mapToSource(proxyPersistentIndex);
                    Q_ASSERT(srcPersistentIndex.isValid());
                    layoutChangePersistentIndexes << srcPersistentIndex;
                }
            });

    connect(&*m_model, &FSModel::layoutChanged,
            [this](const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint) {
                for (int i = 0; i < proxyIndexes.size(); ++i) {
                    changePersistentIndex(proxyIndexes.at(i), mapFromSource(layoutChangePersistentIndexes.at(i)));
                }
                layoutChangePersistentIndexes.clear();
                proxyIndexes.clear();

                QList<QPersistentModelIndex> parents;
                parents.reserve(sourceParents.size());
                for (const QPersistentModelIndex &parent: sourceParents) {
                    if (!parent.isValid()) {
                        parents << QPersistentModelIndex();
                        continue;
                    }
                    const QModelIndex mappedParent = mapFromSource(parent);
                    Q_ASSERT(mappedParent.isValid());
                    parents << mappedParent;
                }
                emit layoutChanged(parents, hint);
            });
    void layoutChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(),
                       QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    void layoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(),
                                QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
}

LocalFileSystemModel::~LocalFileSystemModel()
{}

QModelIndex LocalFileSystemModel::buddy(const QModelIndex &idx) const
{
    return mapFromSource(m_model->buddy(mapToSource(idx)));
}

QModelIndex LocalFileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    return mapFromSource(m_model->index(row, column, mapToSource(parent)));
}

QModelIndex LocalFileSystemModel::fsIndex(const QString &path, int column) const
{
    return mapFromSource(m_model->index(path, column));
}

QModelIndex LocalFileSystemModel::parent(const QModelIndex &child) const
{
    return mapFromSource(m_model->parent(mapToSource(child)));
}

QModelIndex LocalFileSystemModel::sibling(int row, int column, const QModelIndex &idx) const
{
    return mapFromSource(m_model->sibling(row, column, mapToSource(idx)));
}

bool LocalFileSystemModel::hasChildren(const QModelIndex &parent) const
{
    return m_model->hasChildren(mapToSource(parent));
}

bool LocalFileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    return m_model->canFetchMore(mapToSource(parent));
}

void LocalFileSystemModel::fetchMore(const QModelIndex &parent)
{
    m_model->fetchMore(mapToSource(parent));
}

int LocalFileSystemModel::rowCount(const QModelIndex &parent) const
{
    return m_model->rowCount(mapToSource(parent));
}

int LocalFileSystemModel::columnCount(const QModelIndex &parent) const
{
    return m_model->columnCount(mapToSource(parent));
}

QString LocalFileSystemModel::workingDirectory() const
{
    return QDir::currentPath();
}

QVariant LocalFileSystemModel::myComputer(int role) const
{
    return m_model->myComputer(role);
}

QVariant LocalFileSystemModel::homePath(int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        return QDir::homePath();
    case Qt::DecorationRole:
        return qApp->style()->standardIcon(QStyle::SP_DirHomeIcon);
    }

    return QVariant();
}


QVariant LocalFileSystemModel::data(const QModelIndex &index, int role) const
{
    return m_model->data(mapToSource(index), role);
}

bool LocalFileSystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return m_model->setData(mapToSource(index), value, role);
}

QVariant LocalFileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return m_model->headerData(section, orientation, role);
}

Qt::ItemFlags LocalFileSystemModel::flags(const QModelIndex &index) const
{
    return m_model->flags(mapToSource(index));
}

void LocalFileSystemModel::sort(int column, Qt::SortOrder order)
{
    m_model->sort(column, order);
}

QStringList LocalFileSystemModel::mimeTypes() const
{
    return m_model->mimeTypes();
}

QMimeData *LocalFileSystemModel::mimeData(const QModelIndexList &indexes) const
{
    QModelIndexList mapped;
    for (auto &idx: indexes)
        mapped.push_back(mapToSource(idx));
    return m_model->mimeData(mapped);
}

bool LocalFileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                                        const QModelIndex &parent)
{
    return m_model->dropMimeData(data, action, row, column, mapToSource(parent));
}

Qt::DropActions LocalFileSystemModel::supportedDropActions() const
{
    return m_model->supportedDropActions();
}

bool LocalFileSystemModel::isRootDir(const QString &path) const
{
    QDir dir(path);
    return dir.isRoot();
}

QModelIndex LocalFileSystemModel::setRootPath(const QString &path)
{
    return mapFromSource(m_model->setRootPath(path));
}

QString LocalFileSystemModel::rootPath() const
{
    return m_model->rootPath();
}

QDir LocalFileSystemModel::rootDirectory() const
{
    return m_model->rootDirectory();
}

void LocalFileSystemModel::setIconProvider(RemoteFileIconProvider *provider)
{}

RemoteFileIconProvider *LocalFileSystemModel::iconProvider() const
{
    return m_fileIconProvider.get();
}

void LocalFileSystemModel::setFilter(QDir::Filters filters)
{
    m_model->setFilter(filters);
}

QDir::Filters LocalFileSystemModel::filter() const
{
    return m_model->filter();
}

void LocalFileSystemModel::setDisableRecursiveSort(bool enable)
{
    //m_model->d_func()->disableRecursiveSort = enable;
}

bool LocalFileSystemModel::disableRecursiveSort() const
{
    //return m_model->d_func()->disableRecursiveSort = enable;
    return true;
}

void LocalFileSystemModel::setResolveSymlinks(bool enable)
{
    m_model->setResolveSymlinks(enable);
}

bool LocalFileSystemModel::resolveSymlinks() const
{
    return m_model->resolveSymlinks();
}

void LocalFileSystemModel::setReadOnly(bool enable)
{
    m_model->setReadOnly(enable);
}

bool LocalFileSystemModel::isReadOnly() const
{
    return m_model->isReadOnly();
}

void LocalFileSystemModel::setNameFilterDisables(bool enable)
{
    return m_model->setNameFilterDisables(enable);
}

bool LocalFileSystemModel::nameFilterDisables() const
{
    return m_model->nameFilterDisables();
}

void LocalFileSystemModel::setNameFilters(const QStringList &filters)
{
    m_model->setNameFilters(filters);
}

QStringList LocalFileSystemModel::nameFilters() const
{
    return m_model->nameFilters();
}

QString LocalFileSystemModel::filePath(const QModelIndex &index) const
{
    return m_model->filePath(mapToSource(index));
}

bool LocalFileSystemModel::isDir(const QModelIndex &index) const
{
    return m_model->isDir(mapToSource(index));
}

qint64 LocalFileSystemModel::size(const QModelIndex &index) const
{
    return m_model->size(mapToSource(index));
}

QString LocalFileSystemModel::type(const QModelIndex &index) const
{
    return m_model->type(mapToSource(index));
}

QDateTime LocalFileSystemModel::lastModified(const QModelIndex &index) const
{
    return m_model->lastModified(mapToSource(index));
}

QModelIndex LocalFileSystemModel::mkdir(const QModelIndex &parent, const QString &name)
{
    return mapFromSource(m_model->mkdir(parent, name));
}

bool LocalFileSystemModel::rmdir(const QModelIndex &index)
{
    return m_model->rmdir(mapToSource(index));
}

QString LocalFileSystemModel::fileName(const QModelIndex &index) const
{
    return m_model->fileName(mapToSource(index));
}

QIcon LocalFileSystemModel::fileIcon(const QModelIndex &index) const
{
    return m_model->fileIcon(mapToSource(index));
}

QFileDevice::Permissions LocalFileSystemModel::permissions(const QModelIndex &index) const
{
    return m_model->permissions(mapToSource(index));
}

FileInfo LocalFileSystemModel::fileInfo(const QModelIndex &index) const
{
    return FileInfo(m_model->fileInfo(mapToSource(index)));
}

bool LocalFileSystemModel::remove(const QModelIndex &index)
{
    return m_model->remove(mapToSource(index));
}

QModelIndex LocalFileSystemModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!m_model || !sourceIndex.isValid())
        return QModelIndex();

    Q_ASSERT(sourceIndex.model() == &*m_model);
    return createIndex(sourceIndex.row(), sourceIndex.column(), sourceIndex.internalPointer());
}

QModelIndex LocalFileSystemModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!m_model || !proxyIndex.isValid())
        return QModelIndex();
    Q_ASSERT(proxyIndex.model() == this);
    return m_model->createIndex(proxyIndex.row(), proxyIndex.column(), proxyIndex.internalPointer());
}


#if 0
void LocalFileSystemModel::timerEvent(QTimerEvent *event)
{
    m_model->timerEvent(event);
}

bool LocalFileSystemModel::event(QEvent *event)
{
    return m_model->event(event);
}
#endif

#include "moc_localfilesystemmodel.cpp"
