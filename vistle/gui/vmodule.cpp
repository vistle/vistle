#include "vmodule.h"
#include "varrow.h"

/*!
 * \brief VModule::VModule
 * \param parent
 * \param name
 *
 * \todo move the generation of the slots and the main shape out of the constructor
 */
VModule::VModule(QGraphicsItem *parent, QString name) : QGraphicsPolygonItem(parent)
{
    modName = name;
    modIsVisited = false;

    // get the pixel width of the string
    QFont font("times", 30);
    QFontMetrics fm(font);
    w = fm.boundingRect(modName).width() + 20;
    h = fm.boundingRect(modName).height() + 20;

    // Calculate the crucial dimensions based off the width and height
    xAddr = w / 2;
    yAddr = h / 2;

    // Set the points of the polygon. Currently a diamond
    QVector<QPointF> points = { QPointF(0.0, 0.0),
                                QPointF(xAddr, yAddr),
                                QPointF(0.0, yAddr * 2),
                                QPointF(-xAddr, yAddr) };
    baseShape = QPolygonF(points);
    setPolygon(baseShape);

    // create a status object, currently a circle
    //statusShape = new QGraphicsPolygonItem(QPolygonF(QRectF(QPointF(xAddr / 2 - 15, yAddr * 1.5 - 15),
    //                                                        QPointF(xAddr / 2, yAddr * 1.5))), this);
    statusShape = new QGraphicsPolygonItem(QPolygonF(QRectF(QPointF(xAddr / 2 - 15, yAddr * 1.5 - 15),
                                                  QPointF(xAddr / 2, yAddr * 1.5))), this);
    //statusShape.setPolygon(QRectF(QPointF(xAddr / 2 - 15, yAddr * 1.5 - 15), QPointF(xAddr / 2, yAddr * 1.5)));
    statusShape->setToolTip(QString::number(myStatus));

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setToolTip(modName);
    setCursor(Qt::OpenHandCursor);

    createSlots();
    createActions();
    createMenus();
}

VModule::~VModule()
{
    slotList.clear();
    clearArrows();
    parentModules.clear();
    childModules.clear();
    paramModules.clear();
    delete statusShape;
    delete moduleMenu;
    delete copyAct;
    delete deleteAct;
    delete deleteConnectionAct;
}

/*!
 * \brief VModule::copy
 *
 * \todo add copy functionality. Need some sort of buffer to hold any modules that are copied.
 * \todo should the copy action be done in the module? Or passed up to the scene?
 */
void VModule::copy()
{

}

/*!
 * \brief VModule::deleteModule
 */
void VModule::deleteModule()
{
    qDebug()<<"It works!";
}

/*!
 * \brief VModule::deleteConnections
 */
void VModule::deleteConnections()
{
    clearArrows();
}

/*!
 * \brief VModule::createSlots create the connection slots for the module.
 */
void VModule::createSlots()
{
    // Set the points of the input, output, and parameter shapes. Currently triangles
    QVector<QPointF> points = { QPointF(-xAddr, yAddr),
               QPointF(-xAddr / 2, yAddr * 1.5),
               QPointF(-xAddr / 2, yAddr / 2) };
    slotList.append(new VSlot(QPolygonF(points), V_INPUT, this));

    points = { QPointF(xAddr, yAddr),
               QPointF(xAddr / 2, yAddr * 1.5),
               QPointF(xAddr / 2, yAddr / 2) };
    slotList.append(new VSlot(QPolygonF(points), V_OUTPUT, this));

    points = { QPointF(0,0),
               QPointF(xAddr / 2, yAddr / 2),
               QPointF(-xAddr / 2, yAddr / 2) };
    slotList.append(new VSlot(QPolygonF(points), V_PARAMETER, this));

    points =  { QPointF(0, yAddr * 2),
                QPointF(-xAddr / 2, yAddr * 1.5),
                QPointF(xAddr / 2, yAddr * 1.5) };
    slotList.append(new VSlot(QPolygonF(points), V_PARAMETER, this));
}

/*!
 * \brief VModule::createActions
 */
void VModule::createActions()
{
    copyAct = new QAction("Copy", this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip("Copy the module, and all of its properties");
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));

    deleteAct = new QAction("Delete", this);
    deleteAct->setShortcuts(QKeySequence::Delete);
    deleteAct->setStatusTip("Delete the module and all of its connections");
    connect(deleteAct, SIGNAL(triggered()), this, SLOT(deleteModule()));

    deleteConnectionAct = new QAction("Delete All Connections", this);
    deleteConnectionAct->setStatusTip(tr("Delete the module's connections"));
    connect(deleteConnectionAct, SIGNAL(triggered()), this, SLOT(deleteConnections()));
}

/*!
 * \brief VModule::createMenus
 */
void VModule::createMenus()
{
    moduleMenu = new QMenu;
    moduleMenu->addAction(copyAct);
    moduleMenu->addAction(deleteAct);
    moduleMenu->addSeparator();
    moduleMenu->addAction(deleteConnectionAct);

}

/*!
 * \brief VModule::boundingRect
 * \return
 */
QRectF VModule::boundingRect() const
{
    // The bounding area right now is a rectangle simply corresponding to the basic dimensions...
    //  can this be improved to exactly correspond to the area?
    return QRectF(-xAddr, 0.0, w, h);

}

/*!
 * \brief VModule::paint
 * \param painter
 * \param option
 * \param widget
 */
void VModule::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    //VSlot *selectedSlot;

    // Determine various drawing options here, including color. To be implemented later
    QBrush *brush = new QBrush(Qt::gray, Qt::SolidPattern);
    QBrush *highlightBrush = new QBrush(Qt::yellow, Qt::SolidPattern);
    QPen *highlightPen = new QPen(*highlightBrush, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    if (isSelected()) { painter->setPen(*highlightPen); }

    painter->setBrush(*brush);
    painter->drawPolygon(baseShape, Qt::OddEvenFill);

    switch (myStatus) {
        case V_ACTIVE:
            brush->setColor(Qt::green);
            break;
        case V_WAITING:
            brush->setColor(Qt::yellow);
            break;
        case V_PASSIVE:
            brush->setColor(Qt::black);
            break;
        case V_ERROR:
            brush->setColor(Qt::red);
            break;
    }

    //painter->drawRect(QRectF(QPointF(xAddr / 2 - 15, yAddr * 1.5 - 15), QPointF(xAddr / 2, yAddr * 1.5)));
    painter->setBrush(*brush);
    painter->drawPolygon(statusShape->polygon());
    // Draw the text.
    //painter->drawText(10, 35, "Hello World!");
    //painter->drawText(10, 55, "Hello Again, World!");

    // Draw the slots.
    foreach (VSlot *slot, slotList) {
        switch (slot->slot()) {
            case V_INPUT:
                brush->setColor(Qt::red);
                break;
            case V_OUTPUT:
                brush->setColor(Qt::blue);
                break;
            case V_PARAMETER:
                brush->setColor(Qt::black);
                break;
            case V_DEFAULT:
            case V_ERROR:
                // something has gone horribly wrong.
                break;
        }

        painter->setBrush(*brush);
        painter->drawPolygon(slot->polygon());
    }

    painter->setPen(Qt::black);
    painter->drawText(QPointF(-xAddr / 2.25, yAddr), modName);
}

/*!
 * \brief VModule::getSlot returns the slot that was clicked on.
 * \param pos
 * \param slotType
 * \return
 *
 * \todo improve the method header, are an int reference and the slot point both really needed?
 */
VSlot *VModule::getSlot(QPointF pos, int &slotType)
{
    //VSlot *selectedSlot;
    slotType = V_DEFAULT;
    QPointF mappedPos = mapFromScene(pos);

    // The click could be inside the bound but not the shape, test for that
    if (baseShape.containsPoint(mappedPos, Qt::OddEvenFill)) {
        slotType = V_MAIN;
    }

    foreach (VSlot *slot, slotList) {
        if (slot->polygon().containsPoint(mappedPos, Qt::OddEvenFill)) {
            slotType = slot->slot();
            return slot;
        }
    }
}

/*!
 * \brief VModule::itemChange
 * \param change
 * \param value
 * \return QVariant
 */
QVariant VModule::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionChange) {
        foreach (VArrow *arrow, arrowList) {
            arrow->updatePosition();
        }
    }

    return value;
}

///\todo move the creation of actions and menus out of the event!
///      this should be done in createActions() and createMenus()
void VModule::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    moduleMenu->popup(event->screenPos());
}

/*!
 * \brief VModule::addArrow
 * \param arrow
 */
void VModule::addArrow(VArrow *arrow)
{
    arrowList.append(arrow);
}

/*!
 * \brief VModule::removeArrow
 * \param arrow
 */
void VModule::removeArrow(VArrow *arrow)
{
    int index = arrowList.indexOf(arrow);
    if (index != -1) { arrowList.removeAt(index); }
}

/*!
 * \brief VModule::clearArrows
 */
void VModule::clearArrows()
{
    foreach (VArrow *arrow, arrowList) {
        arrow->startItem()->removeArrow(arrow);
        arrow->endItem()->removeArrow(arrow);
        scene()->removeItem(arrow);
        delete arrow;
    }

}

bool VModule::removeParent(VModule *parentMod)
{
    VModule *mod;
    QList<VModule *>::iterator it;
    for (it = parentModules.begin(); it != parentModules.end(); ++it) {
        mod = *it;
        if (mod == parentMod) {
            parentModules.erase(it);
            return true;
        }
    }
    return false;
}

bool VModule::removeChild(VModule *childMod)
{
    VModule *mod;
    QList<VModule *>::iterator it;
    for (it = childModules.begin(); it != childModules.end(); ++it) {
        mod = *it;
        if (mod == childMod) {
            childModules.erase(it);
            return true;
        }
    }
    return false;

}

bool VModule::removeParameter(VModule *paramMod)
{
    VModule *mod;
    QList<VModule *>::iterator it;
    for (it = paramModules.begin(); it != paramModules.end(); ++it) {
        mod = *it;
        if (mod == paramMod) {
            paramModules.erase(it);
            return true;
        }
    }
    return false;

}

/*!
 * \brief VModule::slotPos
 * \param slot
 * \return
 */
QPointF VModule::slotPos(VSlot *slot)
{
    foreach (VSlot *mySlot, slotList) {
        if (mySlot == slot) {
            return mySlot->polygon().first();
        }
    }
}
