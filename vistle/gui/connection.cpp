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
#define _USE_MATH_DEFINES // for M_PI on windows
#include "connection.h"
#include "module.h"
#include "dataflownetwork.h"

#include <QLine>
#include <QPen>
#include <QPoint>

namespace gui {

Connection::Connection(Port *startPort, Port *endPort, State state, QGraphicsItem *parent)
: Base(parent), m_highlight(false), m_state(state), m_color(Qt::black), m_source(startPort), m_destination(endPort)
{
    if (startPort->portType() == Port::Parameter && endPort->portType() == Port::Parameter)
        m_connectionType = Port::Parameter;
    else
        m_connectionType = Port::Output;

    setPen(QPen(m_color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setCursor(Qt::CrossCursor);
    setState(m_state);

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

    if (!isHighlighted()) {
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
}

bool Connection::isHighlighted() const
{
    return m_highlight;
}

void Connection::setHighlight(bool highlight)
{
    m_highlight = highlight;
    if (isHighlighted())
        setColor(scene()->highlightColor());
    else
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
    QLineF line(mapFromScene(m_source->scenePos()), mapFromScene(m_destination->scenePos()));
    setLine(line);
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
    // Set the pen and the brush
    QPen m_Pen = pen();
    m_Pen.setColor(m_color);
    painter->setPen(m_Pen);
    painter->setBrush(m_color);

    // find the position for drawing the connectionhead
    QPointF startPoint = mapFromScene(m_source->scenePos());
    QPointF endPoint = mapFromScene(m_destination->scenePos());
    QLineF centerLine(endPoint, startPoint);

    ///\todo re-implement collision calculations, incorporating proper positions
    setLine(centerLine);
    //setLine(QLineF(intersectPoint, m_StartItem->pos()));

    // change the angle depending on the orientation of the two objects
    double angle = ::acos(line().dx() / line().length());
    if (line().dy() >= 0) {
        angle = (M_PI * 2) - angle;
    } else {
        angle = (M_PI * 2) + angle;
    }

    painter->drawLine(line());

    ///\todo fix selection mechanism in the scene, or don't use functionality.
    /*
    if (isSelected()) {
        painter->setPen(QPen(m_Color, 1, Qt::DashLine));
    }

    QLineF m_Line = line();
    m_Line.translate(0, 4.0);
    painter->drawLine(m_Line);
    m_Line.translate(0, -8.0);
    painter->drawLine(m_Line);
    */
}

} //namespace gui
