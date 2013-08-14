#ifndef VARROW_H
#define VARROW_H

#include <QPainterPath>
#include <QColor>
#include <QRect>
#include <QPainter>

#include "math.h"
#include "consts.h"
#include "port.h"

namespace gui {

class Module;

class Arrow : public QGraphicsLineItem
{
public:
    Arrow(Module *startItem, Module *endItem, Port *startSlot, Port *endSlot, bool needsArrow, int connType);
    QRectF boundingRect() const;                    // re-implemented
    QPainterPath shape() const;                     // re-implemented
    void setColor(const QColor &color)
        { myColor = color; }
    Module *startItem() const
        { return myStartItem; }
    Module *endItem() const
        { return myEndItem; }
    void updatePosition();                          // re-implemented
    int type() const { return TypeArrowItem; }
    int connectionType() { return arrowType; }

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);                    // re-implemented

private:
    Module *myStartItem;                            // starting point of the arrow
    Module *myEndItem;                              // ending point of the arrow
    QColor myColor;                                 // color of the arrow
    QPolygonF arrowHead;                            // arrow head that is drawn on collision with the module
    Port *myStartSlot;
    Port *myEndSlot;
    bool needsArrowHead;                            // not all arrows need the arrowhead.
    int arrowType;
};

#endif // VARROW_H

} //namespace gui
