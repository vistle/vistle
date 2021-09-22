#include "vistlefilesystemmodel.h"

VistleFileSystemModel::VistleFileSystemModel(QObject *parent): QFileSystemModel(parent)
{}

VistleFileSystemModel::~VistleFileSystemModel()
{}

void VistleFileSystemModel::setHub(int hubId)
{
    m_hub = hubId;
}

int VistleFileSystemModel::hub() const
{
    return m_hub;
}

QModelIndex VistleFileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    return QFileSystemModel::index(row, column, parent);
}

QModelIndex VistleFileSystemModel::index(const QString &path, int column) const
{
    return QFileSystemModel::index(path, column);
}

QModelIndex VistleFileSystemModel::parent(const QModelIndex &child) const
{
    return QFileSystemModel::parent(child);
}

QModelIndex VistleFileSystemModel::sibling(int row, int column, const QModelIndex &idx) const
{
    return QFileSystemModel::sibling(row, column, idx);
}

bool VistleFileSystemModel::hasChildren(const QModelIndex &parent) const
{
    return QFileSystemModel::hasChildren(parent);
}

bool VistleFileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    return QFileSystemModel::canFetchMore(parent);
}

void VistleFileSystemModel::fetchMore(const QModelIndex &parent)
{
    QFileSystemModel::fetchMore(parent);
}

int VistleFileSystemModel::rowCount(const QModelIndex &parent) const
{
    return QFileSystemModel::rowCount(parent);
}

int VistleFileSystemModel::columnCount(const QModelIndex &parent) const
{
    return QFileSystemModel::columnCount(parent);
}

QVariant VistleFileSystemModel::myComputer(int role) const
{
    return QFileSystemModel::myComputer(role);
}

QVariant VistleFileSystemModel::data(const QModelIndex &index, int role) const
{
    return QFileSystemModel::data(index, role);
}

bool VistleFileSystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return QFileSystemModel::setData(index, value, role);
}

QVariant VistleFileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QFileSystemModel::headerData(section, orientation, role);
}

Qt::ItemFlags VistleFileSystemModel::flags(const QModelIndex &index) const
{
    return QFileSystemModel::flags(index);
}

void VistleFileSystemModel::sort(int column, Qt::SortOrder order)
{
    QFileSystemModel::sort(column, order);
}

QStringList VistleFileSystemModel::mimeTypes() const
{
    return QFileSystemModel::mimeTypes();
}

QMimeData *VistleFileSystemModel::mimeData(const QModelIndexList &indexes) const
{
    return QFileSystemModel::mimeData(indexes);
}

bool VistleFileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                                         const QModelIndex &parent)
{
    return QFileSystemModel::dropMimeData(data, action, row, column, parent);
}

Qt::DropActions VistleFileSystemModel::supportedDropActions() const
{
    return QFileSystemModel::supportedDropActions();
}

#include "moc_vistlefilesystemmodel.cpp"
