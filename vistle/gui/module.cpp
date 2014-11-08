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
#include "dataflownetwork.h"
#include "mainwindow.h"
#include "dataflowview.h"

namespace gui {

const double Module::portDistance = 3.;

/*!
 * \brief Module::Module
 * \param parent
 * \param name
 *
 * \todo move the generation of the ports and the main shape out of the constructor
 */
Module::Module(QGraphicsItem *parent, QString name)
: Base(parent)
, m_id(vistle::message::Id::Invalid)
, m_Status(SPAWNING)
, m_validPosition(false)
, m_fontHeight(0.)
{
   setName(name);

   setFlag(QGraphicsItem::ItemIsMovable);
   setFlag(QGraphicsItem::ItemIsSelectable);
   setFlag(QGraphicsItem::ItemIsFocusable);
   setFlag(QGraphicsItem::ItemSendsGeometryChanges);
   setCursor(Qt::OpenHandCursor);

   createActions();
   createMenus();
   createGeometry();

   setStatus(m_Status);
}

Module::~Module()
{
    delete m_moduleMenu;
    delete m_execAct;
    delete m_deleteAct;
}

void Module::execModule()
{
   vistle::message::Compute m(m_id, -1);
   vistle::VistleConnection::the().sendMessage(m);
}

/*!
 * \brief Module::deleteModule
 */
void Module::deleteModule()
{
   setStatus(KILLED);
   vistle::message::Kill m(m_id);
   m.setDestId(m_id);
   vistle::VistleConnection::the().sendMessage(m);
}

void Module::createGeometry()
{
   m_color = QColor(100, 200, 200);
}

/*!
 * \brief Module::createActions
 *
 * \todo this doesn't really work at the moment, find out what is wrong
 */
void Module::createActions()
{
    m_deleteAct = new QAction("Delete", this);
    m_deleteAct->setShortcuts(QKeySequence::Delete);
    m_deleteAct->setStatusTip("Delete the module and all of its connections");
    connect(m_deleteAct, SIGNAL(triggered()), this, SLOT(deleteModule()));

    m_execAct = new QAction("Execute", this);
    m_execAct->setStatusTip("Execute the module");
    connect(m_execAct, SIGNAL(triggered()), this, SLOT(execModule()));
}

/*!
 * \brief Module::createMenus
 */
void Module::createMenus()
{
   m_moduleMenu = new QMenu();
   m_moduleMenu->addAction(m_execAct);
   m_moduleMenu->addSeparator();
   m_moduleMenu->addAction(m_deleteAct);
}

void Module::doLayout() {

   prepareGeometryChange();

   // get the pixel width of the string
   QFont font;
   QFontMetrics fm(font);
   QRect textRect = fm.boundingRect(m_name);
   m_fontHeight = textRect.height() + 4*portDistance;

   double w = textRect.width() + 2*portDistance;
   double h = m_fontHeight + 2*portDistance;

   {
      int idx = 0;
      for (Port *in: m_inPorts) {
         in->setPos(portDistance + idx*(portDistance+Port::portSize), 0.);
         ++idx;
      }
      w = qMax(w, 2*portDistance + idx*(portDistance+Port::portSize));

   }

   {
      int idx = 0;
      for (Port *out: m_outPorts) {
         out->setPos(portDistance + idx*(portDistance+Port::portSize), h);
         ++idx;
      }
      w = qMax(w, 2*portDistance + idx*(portDistance+Port::portSize));
   }

   {
      int idx = 0;
      for (Port *param: m_paramPorts) {
         param->setPos(portDistance + idx*(portDistance+Port::portSize), 0);
         param->setVisible(false);
         ++idx;
      }
      //w = qMax(w, 2*portDistance + idx*(portDistance+Port::portSize));
   }

   h += Port::portSize;

   setRect(0., 0., w, h);
}

/*!
 * \brief Module::boundingRect
 * \return the bounding rectangle for the module
 */
QRectF Module::boundingRect() const {

   return rect().united(childrenBoundingRect());
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
   QBrush brush(m_color, Qt::SolidPattern);
   painter->setBrush(brush);

   QPen highlightPen(m_borderColor, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
   if (isSelected() && m_Status != BUSY) {

      QPen pen(scene()->highlightColor(), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
      painter->setPen(pen);
   } else {

      if (m_Status == INITIALIZED) {
         painter->setPen(Qt::NoPen);
      } else {
         painter->setPen(highlightPen);
      }
      painter->setBrush(brush);
   }

   painter->drawRoundedRect(rect(), portDistance, portDistance);

   painter->setPen(Qt::black);
   painter->drawText(QPointF(portDistance, portDistance+Port::portSize+m_fontHeight/2.), m_name);
}

/*!
 * \brief Module::contextMenuEvent
 * \param event
 */
void Module::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
   m_moduleMenu->popup(event->screenPos());
}

void Module::updatePosition(QPointF newPos) const
{
   if (id() != vistle::message::Id::Invalid && isPositionValid()) {
      // don't update until we have our module id
      const double x = newPos.x();
      const double y = newPos.y();
      setParameter("_position", vistle::ParamVector(x, y));
   }
}

void Module::addPort(vistle::Port *port)
{
   Port *guiPort = new Port(port, this);
   m_vistleToGui[port] = guiPort;
   m_guiToVistle[guiPort] = port;

   switch(port->getType()) {
      case vistle::Port::ANY:
         std::cerr << "cannot handle port type ANY" << std::endl;
         break;
      case vistle::Port::INPUT:
         m_inPorts.push_back(guiPort);
         break;
      case vistle::Port::OUTPUT:
         m_outPorts.push_back(guiPort);
         break;
      case vistle::Port::PARAMETER:
         m_paramPorts.push_back(guiPort);
         break;
   }

   doLayout();
}

QString Module::name() const
{
   return m_name;
}

void Module::setName(QString name)
{
   m_name = name;

   doLayout();
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

bool Module::isPositionValid() const {

   return m_validPosition;
}

void Module::setPositionValid() {

   m_validPosition = true;
}

Port *Module::getGuiPort(vistle::Port *port) const {

   const auto &it = m_vistleToGui.find(port);
   if (it == m_vistleToGui.end())
      return nullptr;

   return it->second;
}

vistle::Port *Module::getVistlePort(Port *port) const {

   const auto &it = m_guiToVistle.find(port);
   if (it == m_guiToVistle.end())
      return nullptr;

   return it->second;
}

DataFlowNetwork *Module::scene() const {

   return static_cast<DataFlowNetwork *>(Base::scene());
}

void Module::mousePressEvent(QGraphicsSceneMouseEvent *event) {

   Base::mousePressEvent(event);
}

void Module::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {

   auto p = getParameter<vistle::ParamVector>("_position");
   if (p) {
      vistle::ParamVector v = p->getValue();
      if (v[0] != pos().x() || v[1] != pos().y())
         sendPosition();
   }
   Base::mouseReleaseEvent(event);
}

void Module::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {

   execModule();
   Base::mouseReleaseEvent(event);
}

/*!
 * \brief Module::portPos return the outward point for a port.
 * \param port
 * \return
 *
 * \todo is it necessary to loop through the ports to find this?
 */
QPointF Module::portPos(const Port *port) const
{
   {
      int idx = m_inPorts.indexOf(const_cast<Port *>(port));
      if (idx >= 0) {
         //return QPointF(portDistance + idx*(Port::portSize+portDistance) + 0.5*Port::portSize, 0.5*Port::portSize);
         return QPointF(portDistance + idx*(Port::portSize+portDistance) + 0.5*Port::portSize, 0.0*Port::portSize);
      }
   }

   {
      int idx = m_outPorts.indexOf(const_cast<Port *>(port));
      if (idx >= 0) {
         return QPointF(portDistance + idx*(Port::portSize+portDistance) + 0.5*Port::portSize, rect().bottom()-0.0*Port::portSize);
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
      m_borderColor = Qt::gray;
      break;
   case INITIALIZED:
      toolTip = "Initialized";
      m_borderColor = scene()->highlightColor();
      break;
   case KILLED:
      toolTip = "Killed";
      m_borderColor = Qt::black;
      break;
   case BUSY:
      toolTip = "Busy";
      m_borderColor = QColor(200, 200, 30);
      break;
   case ERROR_STATUS:
      toolTip = "Error";
      m_borderColor = Qt::red;
      break;
   }

   update();
}

} //namespace gui
