#include "arrow.h"
#include "module.h"

#include <QLine>
#include <QPen>
#include <QPoint>

namespace gui {

Arrow::Arrow(Module *startItem, Module *endItem, Port *startSlot, Port *endSlot, bool needsArrow, int connType)
{
    // Do something
    myStartItem = startItem;
    myEndItem = endItem;
    myStartSlot = startSlot;
    myEndSlot = endSlot;
    myColor = Qt::black;
    needsArrowHead = needsArrow;
    arrowType = connType;

    setPen(QPen(myColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setCursor(Qt::OpenHandCursor);
}

/*!
 * \brief Arrow::boundingRect
 * \return
 */
QRectF Arrow::boundingRect() const
{
    // extra selectable area
    qreal extra = (pen().width() + 20) / 2.0;
    return QRectF(line().p1(), QSizeF(line().p2().x() - line().p1().x(),
                                      line().p2().y() - line().p1().y()))
            .normalized()
            .adjusted(-extra, -extra, extra, extra);
}

/*!
 * \brief Arrow::shape
 * \return
 */
QPainterPath Arrow::shape() const
{
    QPainterPath path = QGraphicsLineItem::shape();
    // only add the polygon for the arrow head if it is necessary
    if (needsArrowHead) { path.addPolygon(arrowHead); }

    QPainterPathStroker stroker;
    stroker.setWidth(20);
    return stroker.createStroke(path);
}

/*!
 * \brief Arrow::updatePosition
 *
 * \todo change mapFromItem calls to reflect where they should actually be and connected to which port.
 */
void Arrow::updatePosition()
{
    QLineF line(mapFromItem(myStartItem, 0, 0), mapFromItem(myEndItem, 0, 0));
    setLine(line);
}

/*!
 * \brief Arrow::paint
 * \param painter
 * \param option
 * \param widget
 */
void Arrow::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                   QWidget *widget)
{
    if (myStartItem->collidesWithItem(myEndItem)) { return; }

    // Set the pen and the brush
    QPen myPen = pen();
    myPen.setColor(myColor);
    ///\todo perfect arrow size
    qreal arrowSize = 10;
    painter->setPen(myPen);
    painter->setBrush(myColor);

    // find the position for drawing the arrowhead
    ///\todo change position of the line here
    QPointF startPoint = mapFromItem(myStartItem, myStartItem->portPos(myStartSlot));
    QPointF endPoint = mapFromItem(myEndItem, myEndItem->portPos(myEndSlot));
    QLineF centerLine(endPoint, startPoint);

    ///\todo re-implement collision calculations, incorporating proper positions
    setLine(centerLine);
    //setLine(QLineF(intersectPoint, myStartItem->pos()));

    // change the angle depending on the orientation of the two objects
    double angle = ::acos(line().dx() / line().length());
    if (line().dy() >= 0) { angle = (M_PI * 2) - angle;
    } else                { angle = (M_PI * 2) + angle; }

    painter->drawLine(line());

    // draw the arrow head
    if (needsArrowHead) {
        QPointF arrowP1 = line().p1() + QPointF(sin(angle + M_PI / 3) * arrowSize,
                                                cos(angle + M_PI / 3) * arrowSize);
        QPointF arrowP2 = line().p1() + QPointF(sin(angle + M_PI - M_PI / 3) *arrowSize,
                                                cos(angle + M_PI - M_PI / 3) * arrowSize);
        arrowHead.clear();
        arrowHead << line().p1() << arrowP1 << arrowP2;
        painter->drawPolygon(arrowHead);
    }

    ///\todo fix selection mechanism in the scene, or don't use functionality.
    /*
    if (isSelected()) {
        painter->setPen(QPen(myColor, 1, Qt::DashLine));
    }

    QLineF myLine = line();
    myLine.translate(0, 4.0);
    painter->drawLine(myLine);
    myLine.translate(0, -8.0);
    painter->drawLine(myLine);*/

}

} //namespace gui
