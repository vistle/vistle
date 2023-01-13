/*********************************************************************************/
/*! \file connection.cpp
 *
 * Graphical representation of connections made between modules.
 * the connection class handles the drawing of the connections between two modules.
 *
 * For future reference, code was taken from qt's diagramscene example code, at:
 * http://harmattan-dev.nokia.com/docs/library/html/qt4/graphicsview-diagramscene-connection-cpp.html
 * \todo add license?
 */
/**********************************************************************************/
#include "connection.h"
#include "module.h"
#include "dataflownetwork.h"

#include <QLine>
#include <QPen>
#include <QPoint>
#include <QPainterPathStroker>

namespace gui {

namespace {
float NormalZ = -10;
float HighlightZ = 10;
} // namespace

Connection::Connection(Port *startPort, Port *endPort, State state, QGraphicsItem *parent)
: Base(parent), m_highlight(false), m_state(state), m_color(Qt::black), m_source(startPort), m_destination(endPort)
{
    if (startPort->portType() == Port::Parameter && endPort->portType() == Port::Parameter)
        m_connectionType = Port::Parameter;
    else
        m_connectionType = Port::Output;

    setPen(QPen(m_color, 2.5f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setCursor(Qt::CrossCursor);
    setState(m_state);
    setHighlight(m_highlight);

    setAcceptHoverEvents(true);

    updatePosition();
}

Port *Connection::source() const
{
    return m_source;
}

Port *Connection::destination() const
{
    return m_destination;
}

Connection::State Connection::state() const
{
    return m_state;
}

void Connection::setState(Connection::State state)
{
    m_state = state;

    if (isHighlighted()) {
        setZValue(HighlightZ);
        setColor(scene()->highlightColor());
        return;
    }

    setZValue(NormalZ);

    if (isEmphasized()) {
        if (scene()->isDark()) {
            setColor(scene()->highlightColor().darker());
        } else {
            setColor(scene()->highlightColor().darker(140));
        }
        return;
    }

    switch (m_state) {
    case ToEstablish:
        setColor(QColor(140, 140, 140));
        break;
    case Established:
        setColor(QColor(0, 0, 0));
        break;
    case ToRemove:
        setColor(QColor(200, 50, 50));
        break;
    }
}

void Connection::updateVisibility(int layer)
{
    auto *m1 = source()->module();
    auto *m2 = destination()->module();
    bool v1 = layer < 0 || m1->layer() < 0 || m1->layer() == layer;
    bool v2 = layer < 0 || m2->layer() < 0 || m2->layer() == layer;

    if (LayersAsOpacity) {
        setEnabled(v1 & v2);
        if (v1 && v2) {
            setOpacity(1.0);
        } else {
            setOpacity(0.05);
            setEnabled(false);
        }
    } else {
        setVisible(v1 && v2);
    }
}

bool Connection::isEmphasized() const
{
    return m_emphasized;
}

void Connection::setEmphasis(bool em)
{
    m_emphasized = em;
    setState(m_state);
}

bool Connection::isHighlighted() const
{
    return m_highlight;
}

void Connection::setHighlight(bool highlight)
{
    m_highlight = highlight;
    setState(m_state);
}

void Connection::setColor(const QColor &color)
{
    m_color = color;
    update();
}

/*!
 * \brief Connection::updatePosition
 *
 * \todo change mapFromItem calls to reflect where they should actually be and connected to which port.
 */
void Connection::updatePosition()
{
    auto f = mapFromScene(m_source->scenePos());
    auto t = mapFromScene(m_destination->scenePos());
    QPainterPath path;
    path.moveTo(f);
    float dy = t.y() - f.y();
    float dx = t.x() - f.x();
    const float MinDControl = 2. * Port::portSize;
    float cdy = std::max(MinDControl, dy * .5f);
    if (dy < 0) {
        cdy = std::max(MinDControl, std::min(5.f * MinDControl, std::min(std::abs(dy * .5f), std::abs(dx))));
    }
    if (cdy <= 0) {
        path.lineTo(t);
    } else {
        QPointF cd(0, cdy);
        path.cubicTo(f + cd, t - cd, t);
    }
    QPainterPathStroker s;
    setPath(s.createStroke(path));
}

DataFlowNetwork *Connection::scene() const
{
    return static_cast<DataFlowNetwork *>(Base::scene());
}

void Connection::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    setHighlight(true);
}

void Connection::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    setHighlight(false);
}

void Connection::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    scene()->removeConnection(source(), destination(), true);
}


/*!
 * \brief Connection::paint re-implementation of the paint method
 * \param painter
 * \param option
 * \param widget
 */
void Connection::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    updatePosition();

    // Set the pen and the brush
    QPen m_Pen = pen();
    m_Pen.setColor(m_color);
    painter->setPen(m_Pen);
    painter->setBrush(m_color);

    painter->drawPath(path());
}

} //namespace gui
