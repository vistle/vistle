#ifndef VARROW_H
#define VARROW_H

#include <QGraphicsLineItem>
#include <QPainterPath>
#include <QPen>
#include "math.h"
#include "vconsts.h"
#include "vslot.h"

// Resolves circular dependency
class VModule;

class VArrow : public QGraphicsLineItem
{
public:
    VArrow(VModule *startItem, VModule *endItem, VSlot *startSlot, VSlot *endSlot, bool needsArrow, int connType);
    QRectF boundingRect() const;                    // re-implemented
    QPainterPath shape() const;                     // re-implemented
    void setColor(const QColor &color)
        { myColor = color; }
    VModule *startItem() const
        { return myStartItem; }
    VModule *endItem() const
        { return myEndItem; }
    void updatePosition();                          // re-implemented
    int type() const { return TypeVArrowItem; }
    int connectionType() { return arrowType; }

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);                    // re-implemented

private:
    VModule *myStartItem;                           // starting point of the arrow
    VModule *myEndItem;                             // ending point of the arrow
    QColor myColor;                                 // color of the arrow
    QPolygonF arrowHead;                            // arrow head that is drawn on collision with the module
    VSlot *myStartSlot;
    VSlot *myEndSlot;
    bool needsArrowHead;
    int arrowType;
};

#endif // VARROW_H
