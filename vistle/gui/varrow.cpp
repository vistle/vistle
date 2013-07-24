#include "varrow.h"
#include "vmodule.h"

VArrow::VArrow(VModule *startItem, VModule *endItem, VSlot *startSlot, VSlot *endSlot, bool needsArrow, int connType)
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
 * \brief VArrow::boundingRect
 * \return
 */
QRectF VArrow::boundingRect() const
{
    // extra selectable area
    qreal extra = (pen().width() + 20) / 2.0;
    return QRectF(line().p1(), QSizeF(line().p2().x() - line().p1().x(),
                                      line().p2().y() - line().p1().y()))
            .normalized()
            .adjusted(-extra, -extra, extra, extra);
}

/*!
 * \brief VArrow::shape
 * \return
 */
QPainterPath VArrow::shape() const
{
    QPainterPath path = QGraphicsLineItem::shape();
    // only add the polygon for the arrow head if it is necessary
    if (needsArrowHead) { path.addPolygon(arrowHead); }

    QPainterPathStroker stroker;
    stroker.setWidth(20);
    return stroker.createStroke(path);
}

/*!
 * \brief VArrow::updatePosition
 *
 * \todo change mapFromItem calls to reflect where they should actually be and connected to which slot.
 */
void VArrow::updatePosition()
{
    QLineF line(mapFromItem(myStartItem, 0, 0), mapFromItem(myEndItem, 0, 0));
    setLine(line);
}

/*!
 * \brief VArrow::paint
 * \param painter
 * \param option
 * \param widget
 */
void VArrow::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
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
    QPointF startPoint = mapFromItem(myStartItem, myStartItem->slotPos(myStartSlot));
    QPointF endPoint = mapFromItem(myEndItem, myEndItem->slotPos(myEndSlot));
    QLineF centerLine(endPoint, startPoint);

    /*
    QPolygonF endPolygon = myEndItem->polygon();
    QPointF p1 = endPolygon.first() + myEndItem->pos();
    QPointF p2;
    QPointF intersectPoint;
    QLineF polyLine;
    for (int i = 1; i < endPolygon.count(); ++i) {
        p2 = endPolygon.at(i) + myEndItem->pos();
        polyLine = QLineF(p1, p2);
        QLineF::IntersectType intersectType = polyLine.intersect(centerLine, &intersectPoint);
        if (intersectType == QLineF::BoundedIntersection) {
            //\TODO why was the break statement before the action in the if statement?
            p1 = p2;
            break;
        }
    }*/

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



    /*
     * This functionality for adding dotted lines to indicate arrow selection is broken,
     * and probably not needed.
     */
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
