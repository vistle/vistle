#ifndef VARROW_H
#define VARROW_H

#include <QPainterPath>
#include <QColor>
#include <QRect>
#include <QPainter>

#include "port.h"

namespace gui {

class Module;
class DataFlowNetwork;

class Connection: public QGraphicsPathItem {
    Q_INTERFACES(QGraphicsItem)
    typedef QGraphicsPathItem Base;

public:
    enum State {
        ToEstablish,
        Established,
        ToRemove,
    };

    Connection(Port *startPort, Port *endPort, State state, QGraphicsItem *parent = nullptr);

    Port *source() const;
    Port *destination() const;

    State state() const;
    void setState(State state);

    bool isHighlighted() const;
    void setHighlight(bool highlight);

    bool isEmphasized() const;
    void setEmphasis(bool em);

    void setColor(const QColor &color);
    void updatePosition(); // re-implemented
    int connectionType() { return m_connectionType; }

    DataFlowNetwork *scene() const;

    void updateVisibility(int layer);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget); // re-implemented

private:
    bool m_emphasized = false;
    bool m_highlight = false;
    State m_state;
    QColor m_color; // color of the connection
    Port *m_source;
    Port *m_destination;
    int m_connectionType;
};

} //namespace gui
#endif // VARROW_H
