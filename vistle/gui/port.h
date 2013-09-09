#ifndef VSLOT_H
#define VSLOT_H

#include <QPolygonF>
#include <QGraphicsItem>

namespace gui {

class Port : public QGraphicsPolygonItem
{
public:
   enum Type { DEFAULT,
               MAIN,
               INPUT,
               OUTPUT,
               PARAMETER };

    Port(QPolygonF item, Type port, QGraphicsItem *parent);
    Port();
    Type port() { return portType; }

private:
    Type portType;						//< type of port
};

} //namespace gui

Q_DECLARE_METATYPE(gui::Port::Type)
#endif // VSLOT_H
