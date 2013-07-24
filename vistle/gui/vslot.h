#ifndef VSLOT_H
#define VSLOT_H

#include <QPolygonF>
#include <QGraphicsItem>
#include "vconsts.h"

class VSlot : public QGraphicsPolygonItem
{
public:
    VSlot(QPolygonF item, int slot, QGraphicsItem *parent);
    VSlot();
    int slot()
        { return slotType; }
    int type() const { return TypeVSlotItem; }

private:
    int slotType;                               // type of slot
};

#endif // VSLOT_H
