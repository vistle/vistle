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

class Connection : public QGraphicsLineItem
{
public:
    Connection(Module *startItem, Module *endItem, Port *startPort, Port *endPort, bool needsConnection, int connType);
    QRectF boundingRect() const;                    // re-implemented
    QPainterPath shape() const;                     // re-implemented
    void setColor(const QColor &color)
        { m_Color = color; }
    Module *startItem() const
        { return m_StartItem; }
    Module *endItem() const
        { return m_EndItem; }
    void updatePosition();                          // re-implemented
    int type() const { return TypeConnectionItem; }
    int connectionType() { return m_connectionType; }

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);                    // re-implemented

private:
    Module *m_StartItem;                            // starting point of the connection
    Module *m_EndItem;                              // ending point of the connection
    QColor m_Color;                                 // color of the connection
    QPolygonF connectionHead;                       // connection head that is drawn on collision with the module
    Port *m_StartSlot;
    Port *m_EndSlot;
    bool needsConnectionHead;                        // not all connections need the arrowhead.
    int m_connectionType;
};

} //namespace gui
#endif // VARROW_H

