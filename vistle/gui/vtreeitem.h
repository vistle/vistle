#ifndef VTREEITEM_H
#define VTREEITEM_H

#include <QList>
#include <QVariant>

#if 0

class VTreeItem
{
public:
    VTreeItem(const QList<QVariant> &data, VTreeItem *parent = 0);
    ~VTreeItem();

    void appendChild(VTreeItem *child);

    VTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    VTreeItem *parent();

private:
    QList<VTreeItem *> childItems;
    QList<QVariant> itemData;
    VTreeItem *parentItem;
};

#endif // VTREEITEM_H

#endif
