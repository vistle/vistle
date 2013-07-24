#include "vtreemodel.h"
#include <QtGui>
#include "vtreeitem.h"

#if 0

VTreeModel::VTreeModel(const QString &data, QObject *parent) : QAbstractItemModel(parent)
{
    QList<QVariant> rootData;
    rootData << "Title" << "Summary";
    rootItem = new VTreeItem(rootData);
    setupModelData(data.split(QString("\n")), rootItem);
}

VTreeModel::~VTreeModel()
{
    delete rootItem;
}

int VTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return static_cast<VTreeItem *>(parent.internalPointer())->columnCount();
    } else {
        return rootItem->columnCount();
    }
}

QVariant VTreeModel::data(const QModelIndex &index, int role) const
{
    if (!(index.isValid())) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    VTreeItem *item = static_cast<VTreeItem *>(index.internalPointer());
    return item->data(index.column());
}

Qt::ItemFlags VTreeModel::flags(const QModelIndex &index) const
{
    if (!(index.isValid())) {
        return 0;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant VTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return rootItem->data(section);
    }

    return QVariant();
}

QModelIndex VTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!(hasIndex(row, column, parent))) {
        return QModelIndex();
    }

    VTreeItem *parentItem;

    if (!(parent.isValid())) {
        parentItem = rootItem;
    } else {
        parentItem = static_cast<VTreeItem *>(parent.internalPointer());
    }

    VTreeItem *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    } else {
        return QModelIndex();
    }
}

QModelIndex VTreeModel::parent(const QModelIndex &child) const
{
    if (!(index.isValid())) {
        return QModelIndex();
    }

    VTreeItem *childItem = static_cast<VTreeItem *>(index.internalPointer());
    VTreeItem *parentItem = childItem->parent();

    if (parentItem == rootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int VTreeModel::rowCount(const QModelIndex &parent) const
{
    VTreeItem *parentItem;
    if (parent.column() > 0) {
        return 0;
    }

    if (!(parent.isValid())) {
        parentItem = rootItem;
    } else {
        parentItem = static_cast<VTreeItem *>(parent.internalPointer());
    }

    return parentItem->childCount();
}

void VTreeModel::setupModelData(const QStringList &lines, VTreeItem *parent)
{
    QList<VTreeItem *> parents;
    QList<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].mid(position, 1) != " ") {
                break;
            }
            position++;
        }

        QString lineData = lines[number].mid(position).trimmed();

        if (!(lineData.isEmpty())) {
            QStringList columnStrings = lineData.split("\t", QString::SkipEmptyParts);
            QList<QVariant> columnData;
            for (int column = 0; column < columnStrings.count(); ++column) {
                columnData << columnStrings[column];
            }

            if (position > indentations.last()) {
                if (parents.last()->childCount() > 0) {
                    parents < parents.last()->child(parents.last()->childCount()-1);
                    indentations << position;
                }
            } else {
                while (position < indentations)
}

#endif
