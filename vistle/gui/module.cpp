/*********************************************************************************/
/*! \file module.cpp
 *
 * representation, both graphical and of information, for vistle modules.
 */
/**********************************************************************************/

#include <cassert>

#include <QDebug>
#include <QMenu>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>

#include <userinterface/vistleconnection.h>

#include "module.h"
#include "connection.h"
#include "scene.h"

namespace gui {

/*!
 * \brief Module::Module
 * \param parent
 * \param name
 *
 * \todo move the generation of the ports and the main shape out of the constructor
 */
Module::Module(QGraphicsItem *parent, QString name) : QGraphicsItem(parent)
{

   shape = new QGraphicsPolygonItem(this);


    QString toolTip;
    createPorts();
    setName(name);
    modIsVisited = false;

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setToolTip(m_name);
    setCursor(Qt::OpenHandCursor);

    createActions();
    createMenus();

    setStatus(m_Status);
}

Module::~Module()
{
    portList.clear();
    clearConnections();
    parentModules.clear();
    childModules.clear();
    paramModules.clear();
    delete shape;
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
   setStatus(KILLED);
   vistle::message::Kill m(m_id);
   vistle::VistleConnection::the().sendMessage(m);
}

/*!
 * \brief Module::deleteConnections
 */
void Module::deleteConnections()
{
    clearConnections();
}

/*!
 * \brief Module::createPorts create the connection ports for the module.
 */
void Module::createPorts()
{
    // Set the points of the input, output, and parameter shapes. Currently triangles
    QVector<QPointF> points = { QPointF(-xAddr, yAddr),
               QPointF(-xAddr / 2, yAddr * 1.5),
               QPointF(-xAddr / 2, yAddr / 2) };
    portList.append(new Port(QPolygonF(points), Port::INPUT, this));

    points = { QPointF(xAddr, yAddr),
               QPointF(xAddr / 2, yAddr * 1.5),
               QPointF(xAddr / 2, yAddr / 2) };
    portList.append(new Port(QPolygonF(points), Port::OUTPUT, this));

    points = { QPointF(0,0),
               QPointF(xAddr / 2, yAddr / 2),
               QPointF(-xAddr / 2, yAddr / 2) };
    portList.append(new Port(QPolygonF(points), Port::PARAMETER, this));

    points =  { QPointF(0, yAddr * 2),
                QPointF(-xAddr / 2, yAddr * 1.5),
                QPointF(xAddr / 2, yAddr * 1.5) };
    portList.append(new Port(QPolygonF(points), Port::PARAMETER, this));
}

void Module::updatePorts()
{
   assert (portList.size() == 4);

   QVector<QPointF> points = { QPointF(-xAddr, yAddr),
                               QPointF(-xAddr / 2, yAddr * 1.5),
                               QPointF(-xAddr / 2, yAddr / 2) };
   portList[0]->setPolygon(QPolygonF(points));

   points = { QPointF(xAddr, yAddr),
              QPointF(xAddr / 2, yAddr * 1.5),
              QPointF(xAddr / 2, yAddr / 2) };
   portList[1]->setPolygon(QPolygonF(points));

   points = { QPointF(0,0),
              QPointF(xAddr / 2, yAddr / 2),
              QPointF(-xAddr / 2, yAddr / 2) };
   portList[2]->setPolygon(QPolygonF(points));

   points =  { QPointF(0, yAddr * 2),
               QPointF(-xAddr / 2, yAddr * 1.5),
               QPointF(xAddr / 2, yAddr * 1.5) };
   portList[3]->setPolygon(QPolygonF(points));
}

/*!
 * \brief Module::createActions
 *
 * \todo this doesn't really work at the moment, find out what is wrong
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
 * \return the bounding rectangle for the module
 */
QRectF Module::boundingRect() const
{
   return childrenBoundingRect();
#if 0
    ///\todo The bounding area right now is a rectangle simply corresponding to the basic dimensions... improve?
    return QRectF(-xAddr, 0.0, w, h);
#endif
}

/*!
 * \brief Module::paint
 * \param painter
 * \param option
 * \param widget
 */
void Module::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Determine various drawing options here, including color.
    ///\todo change colors depending on type of module, hostname, etc.
    QBrush *brush = new QBrush(Qt::gray, Qt::SolidPattern);
    QBrush *highlightBrush = new QBrush(Qt::yellow, Qt::SolidPattern);
    QPen *highlightPen = new QPen(*highlightBrush, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    if (isSelected()) { painter->setPen(*highlightPen); }

    painter->setBrush(*brush);
    painter->drawPolygon(baseShape, Qt::OddEvenFill);

    switch (m_Status) {
        case SPAWNING:
            brush->setColor(Qt::gray);
            break;
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
            case Port::INPUT:
                brush->setColor(Qt::red);
                break;
            case Port::OUTPUT:
                brush->setColor(Qt::blue);
                break;
            case Port::PARAMETER:
                brush->setColor(Qt::black);
                break;
            case Port::MAIN:
            case Port::DEFAULT:
                break;
        }

        painter->setBrush(*brush);
        painter->drawPolygon(port->polygon());
    }

    painter->setPen(Qt::black);
    painter->drawText(QPointF(-xAddr / 2.25, yAddr), m_name);
}

/*!
 * \brief Module::contextMenuEvent
 * \param event
 */
void Module::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
   moduleMenu->popup(event->screenPos());
}

QVariant Module::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
   if (change == ItemPositionChange && scene()) {
      // value is the new position.
      QPointF newPos = value.toPointF();
      updatePosition(newPos);
   }

   return QGraphicsItem::itemChange(change, value);
}

void Module::updatePosition(QPointF newPos) const
{
   double x = newPos.x();
   double y = newPos.y();
   setParameter("_x", x);
   setParameter("_y", y);
}

/*!
 * \brief Module::addconnection
 * \param connection
 */
void Module::addConnection(Connection *connection)
{
    connectionList.append(connection);
}

/*!
 * \brief Module::removeconnection
 * \param connection
 */
void Module::removeConnection(Connection *connection)
{
    int index = connectionList.indexOf(connection);
    if (index != -1) { connectionList.removeAt(index); }
}

/*!
 * \brief Module::clearconnections
 */
void Module::clearConnections()
{
    foreach (Connection *connection, connectionList) {
        connection->startItem()->removeConnection(connection);
        connection->endItem()->removeConnection(connection);
        scene()->removeItem(connection);
        delete connection;
    }

}

/*!
 * \brief Module::removeParent
 * \param parentMod
 * \return
 */
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

/*!
 * \brief Module::removeChild
 * \param childMod
 * \return
 */
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

/*!
 * \brief Module::disconnectParameter
 * \param paramMod
 * \return
 */
bool Module::disconnectParameter(Module *paramMod)
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

QString Module::name() const
{
   return m_name;
}

void Module::setName(QString name)
{
   m_name = name;
   setToolTip(m_name);

   // get the pixel width of the string
   QFont font("times", 30);
   QFontMetrics fm(font);
   w = fm.boundingRect(m_name).width() + 20;
   h = fm.boundingRect(m_name).height() + 20;

   // Calculate the crucial dimensions based off the width and height
   xAddr = w / 2;
   yAddr = h / 2;

   // Set the points of the polygon. Currently a diamond
   QVector<QPointF> points = { QPointF(0.0, 0.0),
                               QPointF(xAddr, yAddr),
                               QPointF(0.0, yAddr * 2),
                               QPointF(-xAddr, yAddr) };
   baseShape = QPolygonF(points);
   shape->setPolygon(baseShape);

   delete statusShape;
   statusShape = new QGraphicsPolygonItem(QPolygonF(QRectF(QPointF(xAddr / 2 - 15, yAddr * 1.5 - 15),
                                                           QPointF(xAddr / 2, yAddr * 1.5))), this);

   updatePorts();
}

int Module::id() const
{
   return m_id;
}

void Module::setId(int id)
{
   m_id = id;
}

boost::uuids::uuid Module::spawnUuid() const
{
   return m_spawnUuid;
}

void Module::setSpawnUuid(const boost::uuids::uuid &uuid)
{
   m_spawnUuid = uuid;
}

void Module::sendPosition() const
{
   updatePosition(pos());
}

/*!
 * \brief Module::portPos return the outward point for a port.
 * \param port
 * \return
 *
 * \todo is it necessary to loop through the ports to find this?
 */
QPointF Module::portPos(Port *port)
{
    foreach (Port *m_Slot, portList) {
        if (m_Slot == port) {
            return m_Slot->polygon().first();
        }
    }

    return QPointF();
}

void Module::setStatus(Module::Status status)
{
   m_Status = status;

   QString toolTip = "Unknown";

   switch (m_Status) {
   case SPAWNING:
      toolTip = "Spawning";
      break;
   case INITIALIZED:
      toolTip = "Initialized";
      break;
   case KILLED:
      toolTip = "Killed";
      break;
   case BUSY:
      toolTip = "Busy";
      break;
   case ERROR:
      toolTip = "Error";
      break;
   }

   statusShape->setToolTip(toolTip);
}

} //namespace gui
