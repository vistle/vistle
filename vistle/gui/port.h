#ifndef GUI_PORT_H
#define GUI_PORT_H

#include <QGraphicsItem>
#include <memory>

namespace vistle {
class Port;
}

namespace gui {

class Module;

class Port: public QObject, public QGraphicsRectItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    typedef QGraphicsRectItem Base;

public:
    static const double portSize;

    enum Type {
        Parameter,
        Input,
        Output,
    };

    Port(const vistle::Port *port, Module *parent);
    Port(Type type, Module *parent);

    bool valid() const;
    Type portType() const;
    Module *module() const;
    vistle::Port *vistlePort() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QPointF scenePos() const;

    bool operator<(const Port &other) const;
    bool operator==(const Port &other) const;

signals:
    void clicked(Port *port);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
    void createTooltip();
    void createGeometry();

    Type m_portType; //< type of port
    std::shared_ptr<vistle::Port> m_port;
    QColor m_color;
    Module *m_module;
};

} //namespace gui

Q_DECLARE_METATYPE(gui::Port::Type)
#endif // VSLOT_H
