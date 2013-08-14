#ifndef VSLOT_H
#define VSLOT_H

#include <QPolygonF>
#include <QGraphicsItem>
#include "consts.h"

namespace gui {

class Port : public QGraphicsPolygonItem
{
public:
    Port(QPolygonF item, GraphicsType port, QGraphicsItem *parent);
    Port();
    GraphicsType port()
        { return portType; }
    int type() const { return TypePortItem; }

private:
    GraphicsType portType;						// type of port
};

} //namespace gui

#endif // VSLOT_H
