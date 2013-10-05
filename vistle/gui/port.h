#ifndef VSLOT_H
#define VSLOT_H

#include <QGraphicsItem>

namespace vistle {
class Port;
}

namespace gui {

class Module;

class Port: public QObject, public QGraphicsRectItem
{
   Q_OBJECT
   Q_INTERFACES(QGraphicsItem)
   typedef QGraphicsRectItem Base;

public:
   static const double constexpr portSize = 14.;

   enum Type {
      Parameter,
      Input,
      Output,
      };

    Port(vistle::Port *port, Module *parent);
    Port(Type type, Module *parent);

    Type portType() const;
    Module *module() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QPointF scenePos() const;

signals:
    void clicked(Port *port);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
    void createTooltip();
    void createGeometry();

    Type m_portType;						//< type of port
    vistle::Port *m_port = nullptr;
    QColor m_color;
    Module *m_module = nullptr;
};

} //namespace gui

Q_DECLARE_METATYPE(gui::Port::Type)
#endif // VSLOT_H
