#include "abstractfilesystemmodel.h"

AbstractFileSystemModel::AbstractFileSystemModel(QObject *parent): QAbstractItemModel(parent)
{}

AbstractFileSystemModel::~AbstractFileSystemModel()
{}

QHash<int, QByteArray> AbstractFileSystemModel::roleNames() const
{
    auto names = QAbstractItemModel::roleNames();
    names.insertMulti(AbstractFileSystemModel::FileIconRole, QByteArrayLiteral("fileIcon")); // == Qt::decoration
    names.insert(AbstractFileSystemModel::FilePathRole, QByteArrayLiteral("filePath"));
    names.insert(AbstractFileSystemModel::FileNameRole, QByteArrayLiteral("fileName"));
    names.insert(AbstractFileSystemModel::FilePermissions, QByteArrayLiteral("filePermissions"));

    return names;
}

#if 0
QModelIndex AbstractFileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    return QFileSystemModel::index(row, column, parent);
}

QModelIndex AbstractFileSystemModel::index(const QString &path, int column) const
{
    return QFileSystemModel::index(path, column);
}

QModelIndex AbstractFileSystemModel::parent(const QModelIndex &child) const
{
    return QFileSystemModel::parent(child);
}

QModelIndex AbstractFileSystemModel::sibling(int row, int column, const QModelIndex &idx) const
{
    return QFileSystemModel::sibling(row, column, idx);
}

bool AbstractFileSystemModel::hasChildren(const QModelIndex &parent) const
{
    return QFileSystemModel::hasChildren(parent);
}

bool AbstractFileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    return QFileSystemModel::canFetchMore(parent);
}

void AbstractFileSystemModel::fetchMore(const QModelIndex &parent)
{
    QFileSystemModel::fetchMore(parent);
}

int AbstractFileSystemModel::rowCount(const QModelIndex &parent) const
{
    return  QFileSystemModel::rowCount(parent);
}

int AbstractFileSystemModel::columnCount(const QModelIndex &parent) const
{
    return  QFileSystemModel::columnCount(parent);
}

QVariant AbstractFileSystemModel::myComputer(int role) const
{
    return QFileSystemModel::myComputer(role);
}

QVariant AbstractFileSystemModel::data(const QModelIndex &index, int role) const
{
    return QFileSystemModel::data(index, role);
}

bool AbstractFileSystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return QFileSystemModel::setData(index, value, role);
}

QVariant AbstractFileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QFileSystemModel::headerData(section, orientation, role);
}

Qt::ItemFlags AbstractFileSystemModel::flags(const QModelIndex &index) const
{
    return QFileSystemModel::flags(index);
}

void AbstractFileSystemModel::sort(int column, Qt::SortOrder order)
{
    QFileSystemModel::sort(column, order);
}

QStringList AbstractFileSystemModel::mimeTypes() const
{
    return QFileSystemModel::mimeTypes();
}

QMimeData *AbstractFileSystemModel::mimeData(const QModelIndexList &indexes) const
{
    return QFileSystemModel::mimeData(indexes);
}

bool AbstractFileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    return QFileSystemModel::dropMimeData(data, action, row, column, parent);
}

Qt::DropActions AbstractFileSystemModel::supportedDropActions() const
{
    return QFileSystemModel::supportedDropActions();
}
#endif

#include "moc_abstractfilesystemmodel.cpp"
