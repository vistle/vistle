#include "vtreeitem.h"
#include <QStringList>

#if 0

VTreeItem::VTreeItem(const QList<QVariant> &data, VTreeItem *parent)
{
    parentItem = parent;
    itemData = data;
}

VTreeItem::~VTreeItem()
{
    qDeleteAll(childItems);
}

void VTreeItem::appendChild(VTreeItem *child)
{
    childItems.append(child);
}

VTreeItem *VTreeItem::child(int row)
{
    return childItems.value(row);
}

int VTreeItem::childCount() const
{
    return childItems.count();
}

int VTreeItem::columnCount() const
{
    return itemData.count();
}

QVariant VTreeItem::data(int column) const
{
    return itemData.value(column);
}

VTreeItem *VTreeItem::parent()
{
    return parentItem;
}

int VTreeItem::row() const
{
    if (parentItem) {
        return parentItem->childItems.indexOf(const_cast<VTreeItem *>(this));
    }

    return 0;
}

#endif
