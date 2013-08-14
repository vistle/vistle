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

#include <QLine>
#include <QPen>
#include <QPoint>

namespace gui {

Connection::Connection(Module *startItem, Module *endItem, Port *startPort, Port *endPort, bool needsConnection, int connType)
{
    // Do something
    m_StartItem = startItem;
    m_EndItem = endItem;
    m_StartSlot = startPort;
    m_EndSlot = endPort;
    m_Color = Qt::black;
    needsConnectionHead = needsConnection;
    m_connectionType = connType;

    setPen(QPen(m_Color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setCursor(Qt::OpenHandCursor);
}

/*!
 * \brief Connection::boundingRect
 * \return
 */
QRectF Connection::boundingRect() const
{
    // extra selectable area
    qreal extra = (pen().width() + 20) / 2.0;
    return QRectF(line().p1(), QSizeF(line().p2().x() - line().p1().x(),
                                      line().p2().y() - line().p1().y()))
            .normalized()
            .adjusted(-extra, -extra, extra, extra);
}

/*!
 * \brief Connection::shape
 * \return QPainterPath
 */
QPainterPath Connection::shape() const
{
    QPainterPath path = QGraphicsLineItem::shape();
    // only add the polygon for the connection head if it is necessary
    if (needsConnectionHead) { path.addPolygon(connectionHead); }

    QPainterPathStroker stroker;
    stroker.setWidth(20);
    return stroker.createStroke(path);
}

/*!
 * \brief Connection::updatePosition
 *
 * \todo change mapFromItem calls to reflect where they should actually be and connected to which port.
 */
void Connection::updatePosition()
{
    QLineF line(mapFromItem(m_StartItem, 0, 0), mapFromItem(m_EndItem, 0, 0));
    setLine(line);
}

/*!
 * \brief Connection::paint re-implementation of the paint method
 * \param painter
 * \param option
 * \param widget
 */
void Connection::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                   QWidget *widget)
{
    if (m_StartItem->collidesWithItem(m_EndItem)) { return; }

    // Set the pen and the brush
    QPen m_Pen = pen();
    m_Pen.setColor(m_Color);
    ///\todo perfect connection size
    qreal connectionSize = 10;
    painter->setPen(m_Pen);
    painter->setBrush(m_Color);

    // find the position for drawing the connectionhead
    QPointF startPoint = mapFromItem(m_StartItem, m_StartItem->portPos(m_StartSlot));
    QPointF endPoint = mapFromItem(m_EndItem, m_EndItem->portPos(m_EndSlot));
    QLineF centerLine(endPoint, startPoint);

    ///\todo re-implement collision calculations, incorporating proper positions
    setLine(centerLine);
    //setLine(QLineF(intersectPoint, m_StartItem->pos()));

    // change the angle depending on the orientation of the two objects
    double angle = ::acos(line().dx() / line().length());
    if (line().dy() >= 0) { angle = (M_PI * 2) - angle;
    } else                { angle = (M_PI * 2) + angle; }

    painter->drawLine(line());

    // draw the connection head
    if (needsConnectionHead) {
        QPointF connectionP1 = line().p1() + QPointF(sin(angle + M_PI / 3) * connectionSize,
                                                cos(angle + M_PI / 3) * connectionSize);
        QPointF connectionP2 = line().p1() + QPointF(sin(angle + M_PI - M_PI / 3) *connectionSize,
                                                cos(angle + M_PI - M_PI / 3) * connectionSize);
        connectionHead.clear();
        connectionHead << line().p1() << connectionP1 << connectionP2;
        painter->drawPolygon(connectionHead);
    }

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
