#include "module.h"
#include "arrow.h"

#include <QDebug>
#include <QMenu>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>

namespace gui {

/*!
 * \brief Module::Module
 * \param parent
 * \param name
 *
 * \todo move the generation of the ports and the main shape out of the constructor
 */
Module::Module(QGraphicsItem *parent, QString name) : QGraphicsPolygonItem(parent)
{
    QString toolTip;
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

    statusShape = new QGraphicsPolygonItem(QPolygonF(QRectF(QPointF(xAddr / 2 - 15, yAddr * 1.5 - 15),
                                                  QPointF(xAddr / 2, yAddr * 1.5))), this);

    switch (myStatus)
    {
        case INITIALIZED:
            toolTip = "Initialized";
            break;
        case KILLED:
            toolTip = "Killed";
            break;
        case BUSY:
            toolTip = "Busy";
            break;
    }
    statusShape->setToolTip(toolTip);

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

Module::~Module()
{
    portList.clear();
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
 * \brief Module::copy
 *
 * \todo add copy functionality. Need some sort of buffer to hold any modules that are copied.
 * \todo should the copy action be done in the module? Or passed up to the scene?
 */
void Module::copy()
{

}

/*!
 * \brief Module::deleteModule
 */
void Module::deleteModule()
{
    qDebug()<<"It works!";
}

/*!
 * \brief Module::deleteConnections
 */
void Module::deleteConnections()
{
    clearArrows();
}

/*!
 * \brief Module::createSlots create the connection ports for the module.
 */
void Module::createSlots()
{
    // Set the points of the input, output, and parameter shapes. Currently triangles
    QVector<QPointF> points = { QPointF(-xAddr, yAddr),
               QPointF(-xAddr / 2, yAddr * 1.5),
               QPointF(-xAddr / 2, yAddr / 2) };
    portList.append(new Port(QPolygonF(points), INPUT, this));

    points = { QPointF(xAddr, yAddr),
               QPointF(xAddr / 2, yAddr * 1.5),
               QPointF(xAddr / 2, yAddr / 2) };
    portList.append(new Port(QPolygonF(points), OUTPUT, this));

    points = { QPointF(0,0),
               QPointF(xAddr / 2, yAddr / 2),
               QPointF(-xAddr / 2, yAddr / 2) };
    portList.append(new Port(QPolygonF(points), PARAMETER, this));

    points =  { QPointF(0, yAddr * 2),
                QPointF(-xAddr / 2, yAddr * 1.5),
                QPointF(xAddr / 2, yAddr * 1.5) };
    portList.append(new Port(QPolygonF(points), PARAMETER, this));
}

/*!
 * \brief Module::createActions
 */
void Module::createActions()
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
 * \brief Module::createMenus
 */
void Module::createMenus()
{
    moduleMenu = new QMenu;
    moduleMenu->addAction(copyAct);
    moduleMenu->addAction(deleteAct);
    moduleMenu->addSeparator();
    moduleMenu->addAction(deleteConnectionAct);

}

/*!
 * \brief Module::boundingRect
 * \return
 */
QRectF Module::boundingRect() const
{
    // The bounding area right now is a rectangle simply corresponding to the basic dimensions...
    //  can this be improved to exactly correspond to the area?
    return QRectF(-xAddr, 0.0, w, h);

}

/*!
 * \brief Module::paint
 * \param painter
 * \param option
 * \param widget
 */
void Module::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    //Port *selectedSlot;

    // Determine various drawing options here, including color. To be implemented later
    QBrush *brush = new QBrush(Qt::gray, Qt::SolidPattern);
    QBrush *highlightBrush = new QBrush(Qt::yellow, Qt::SolidPattern);
    QPen *highlightPen = new QPen(*highlightBrush, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    if (isSelected()) { painter->setPen(*highlightPen); }

    painter->setBrush(*brush);
    painter->drawPolygon(baseShape, Qt::OddEvenFill);

    switch (myStatus) {
        case INITIALIZED:
            brush->setColor(Qt::green);
            break;
        case KILLED:
            brush->setColor(Qt::red);
            break;
        case BUSY:
            brush->setColor(Qt::yellow);
            break;
        case ERROR:
            brush->setColor(Qt::black);
            break;
    }

    painter->setBrush(*brush);
    painter->drawPolygon(statusShape->polygon());

    // Draw the ports.
    foreach (Port *port, portList) {
        switch (port->port()) {
            case INPUT:
                brush->setColor(Qt::red);
                break;
            case OUTPUT:
                brush->setColor(Qt::blue);
                break;
            case PARAMETER:
                brush->setColor(Qt::black);
                break;
            case DEFAULT:
                break;
        }

        painter->setBrush(*brush);
        painter->drawPolygon(port->polygon());
    }

    painter->setPen(Qt::black);
    painter->drawText(QPointF(-xAddr / 2.25, yAddr), modName);
}

/*!
 * \brief Module::getPort returns the port that was clicked on.
 * \param pos
 * \param portType
 * \return
 *
 * \todo improve the method header, are an int reference and the port point both really needed?
 */
Port *Module::getPort(QPointF pos, int &portType)
{
    //Port *selectedSlot;
    portType = DEFAULT;
    QPointF mappedPos = mapFromScene(pos);

    // The click could be inside the bound but not the shape, test for that
    if (baseShape.containsPoint(mappedPos, Qt::OddEvenFill)) {
        portType = MAIN;
    }

    foreach (Port *port, portList) {
        if (port->polygon().containsPoint(mappedPos, Qt::OddEvenFill)) {
            portType = port->port();
            return port;
        }
    }
}

/*!
 * \brief Module::itemChange
 * \param change
 * \param value
 * \return QVariant
 */
 /*
QVariant Module::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionChange) {
        foreach (Arrow *arrow, arrowList) {
            arrow->updatePosition();
        }
    }

    return value;
}*/

///\todo move the creation of actions and menus out of the event!
///      this should be done in createActions() and createMenus()
void Module::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    moduleMenu->popup(event->screenPos());
}

/*!
 * \brief Module::addArrow
 * \param arrow
 */
void Module::addArrow(Arrow *arrow)
{
    arrowList.append(arrow);
}

/*!
 * \brief Module::removeArrow
 * \param arrow
 */
void Module::removeArrow(Arrow *arrow)
{
    int index = arrowList.indexOf(arrow);
    if (index != -1) { arrowList.removeAt(index); }
}

/*!
 * \brief Module::clearArrows
 */
void Module::clearArrows()
{
    foreach (Arrow *arrow, arrowList) {
        arrow->startItem()->removeArrow(arrow);
        arrow->endItem()->removeArrow(arrow);
        scene()->removeItem(arrow);
        delete arrow;
    }

}

bool Module::removeParent(Module *parentMod)
{
    Module *mod;
    QList<Module *>::iterator it;
    for (it = parentModules.begin(); it != parentModules.end(); ++it) {
        mod = *it;
        if (mod == parentMod) {
            parentModules.erase(it);
            return true;
        }
    }
    return false;
}

bool Module::removeChild(Module *childMod)
{
    Module *mod;
    QList<Module *>::iterator it;
    for (it = childModules.begin(); it != childModules.end(); ++it) {
        mod = *it;
        if (mod == childMod) {
            childModules.erase(it);
            return true;
        }
    }
    return false;

}

bool Module::removeParameter(Module *paramMod)
{
    Module *mod;
    QList<Module *>::iterator it;
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
 * \brief Module::portPos
 * \param port
 * \return
 */
QPointF Module::portPos(Port *port)
{
    foreach (Port *mySlot, portList) {
        if (mySlot == port) {
            return mySlot->polygon().first();
        }
    }
}

} //namespace gui
