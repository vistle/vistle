/*********************************************************************************/
/** \file scene.cpp
 *
 * The brunt of the graphical processing and UI interaction happens here --
 * - modules and connections are created and modified
 * - the brunt of the event handling is done
 * - any interactions between modules, such as sorting, is performed
 */
/**********************************************************************************/
#include "dataflownetwork.h"
#include "module.h"
#include "connection.h"
#include "vistleconsole.h"

#include <core/statetracker.h>

#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

namespace gui {

/*!
 * \brief Scene::Scene
 * \param parent
 */
DataFlowNetwork::DataFlowNetwork(vistle::VistleConnection *conn, QObject *parent)
: QGraphicsScene(parent)
, m_Line(nullptr)
, startPort(nullptr)
, startModule(nullptr)
, endModule(nullptr)
, m_vistleConnection(conn)
, m_state(conn->ui().state())
{
    // Initialize starting scene information.
    m_LineColor = Qt::black;
    m_mousePressed = false;

    m_highlightColor = QColor(200, 50, 200);
}

/*!
 * \brief Scene::~Scene
 */
DataFlowNetwork::~DataFlowNetwork()
{
    m_moduleList.clear();
}

/*!
 * \brief Scene::addModule add a module to the draw area.
 * \param modName
 * \param dropPos
 */
void DataFlowNetwork::addModule(int hub, QString modName, QPointF dropPos)
{
    Module *module = new Module(0, modName);
    ///\todo improve how the data such as the name is set in the module.
    addItem(module);
    module->setPos(dropPos);
    module->setPositionValid();
    module->setStatus(Module::SPAWNING);

    vistle::message::Spawn spawnMsg(hub, modName.toUtf8().constData());
    spawnMsg.setDestId(vistle::message::Id::MasterHub); // to master, for module id generation
    module->setSpawnUuid(spawnMsg.uuid());
    m_vistleConnection->sendMessage(spawnMsg);

    ///\todo add the objects only to the map (sortMap) currently used for sorting, not to the list.
    ///This will remove the need for moduleList altogether
    m_moduleList.append(module);
    module->setHub(vistle::message::Id::MasterHub - hub);
}

void DataFlowNetwork::addModule(int moduleId, const boost::uuids::uuid &spawnUuid, QString name)
{
   //std::cerr << "addModule: name=" << name.toStdString() << ", id=" << moduleId << std::endl;
   Module *mod = findModule(spawnUuid);
   if (mod) {
      mod->sendPosition();
   } else {
      mod = findModule(moduleId);
   }
   if (!mod) {
      mod = new Module(nullptr, name);
      addItem(mod);
      mod->setStatus(Module::SPAWNING);
      m_moduleList.append(mod);
   }

   mod->setId(moduleId);
   int hub = vistle::message::Id::MasterHub - m_state.getHub(moduleId);
   mod->setHub(hub);
   mod->setName(QString("%1_%2").arg(name, QString::number(moduleId)));
}

void DataFlowNetwork::deleteModule(int moduleId)
{
   Module *m = findModule(moduleId);
   if (m) {
      removeItem(m);
      m_moduleList.removeAll(m);
   }
}

void DataFlowNetwork::moduleStateChanged(int moduleId, int stateBits)
{
   if (Module *m = findModule(moduleId)) {
      if (stateBits & vistle::StateObserver::Killed)
         m->setStatus(Module::KILLED);
      else if (stateBits & vistle::StateObserver::Busy)
         m->setStatus(Module::BUSY);
      else if (stateBits & vistle::StateObserver::Initialized)
         m->setStatus(Module::INITIALIZED);
      else
         m->setStatus(Module::SPAWNING);
   }
}

void DataFlowNetwork::newPort(int moduleId, QString portName)
{
   if (Module *m = findModule(moduleId)) {
      const vistle::Port *port = m_state.portTracker()->getPort(moduleId, portName.toStdString());
      if (port) {
         m->addPort(*port);
      }
   }
}

void DataFlowNetwork::deletePort(int moduleId, QString portName) {

#if 0
   QString text = "Delete port: " + QString::number(moduleId) + ":" + portName;
   VistleConsole::the()->appendDebug(text);
   std::cerr << text.toStdString() << std::endl;
#endif
   if (Module *m = findModule(moduleId)) {
      m->removePort(vistle::Port(moduleId, portName.toStdString(), vistle::Port::ANY));
   }
}

void DataFlowNetwork::newConnection(int fromId, QString fromName,
                                   int toId, QString toName) {

#if 0
   QString text = "New Connection: " + QString::number(fromId) + ":" + fromName + " -> " + QString::number(toId) + ":" + toName;
   m_console->appendDebug(text);
#endif

   const vistle::Port *portFrom = m_state.portTracker()->findPort(fromId, fromName.toStdString());
   const vistle::Port *portTo = m_state.portTracker()->findPort(toId, toName.toStdString());

   Module *mFrom = findModule(fromId);
   Module *mTo = findModule(toId);

   if (mFrom && portFrom && mTo && portTo) {
      addConnection(mFrom->getGuiPort(portFrom), mTo->getGuiPort(portTo));
   }
}

void DataFlowNetwork::deleteConnection(int fromId, QString fromName,
                                      int toId, QString toName)
{
#if 0
   QString text = "Connection removed: " + QString::number(fromId) + ":" + fromName + " -> " + QString::number(toId) + ":" + toName;
   m_console->appendDebug(text);
#endif

   const vistle::Port *portFrom = m_state.portTracker()->findPort(fromId, fromName.toStdString());
   const vistle::Port *portTo = m_state.portTracker()->findPort(toId, toName.toStdString());

   Module *mFrom = findModule(fromId);
   Module *mTo = findModule(toId);

   if (mFrom && portFrom && mTo && portTo) {
      removeConnection(mFrom->getGuiPort(portFrom), mTo->getGuiPort(portTo));
   }
}



void DataFlowNetwork::addConnection(Port *portFrom, Port *portTo, bool sendToController) {

   assert(portFrom);
   assert(portTo);

   ConnectionKey key(portFrom, portTo);
   auto it = m_connections.end();
   if (key.valid())
       it = m_connections.find(key);
   Connection *c = nullptr;
   if (it != m_connections.end()) {
      qDebug() << "already have connection";
      c = it->second;
   } else {
      //qDebug() << "new connection";
      c = new Connection(portFrom, portTo, sendToController ? Connection::ToEstablish : Connection::Established);
      m_connections[key] = c;
      addItem(c);
   }

   if (sendToController) {

      const vistle::Port *vFrom = portFrom->module()->getVistlePort(portFrom);
      const vistle::Port *vTo = portTo->module()->getVistlePort(portTo);
      m_vistleConnection->connect(vFrom, vTo);
   } else {
      c->setState(Connection::Established);
   }
}

void DataFlowNetwork::removeConnection(Port *portFrom, Port *portTo, bool sendToController)
{
   ConnectionKey key(portFrom, portTo);
   assert(key.valid());
   auto it = m_connections.find(key);
   if (it == m_connections.end()) {
      std::cerr << "connection to be removed not found" << std::endl;
      return;
   }
   Connection *c = it->second;

   if (sendToController) {
      c->setState(Connection::ToRemove);
      const vistle::Port *vFrom = portFrom->module()->getVistlePort(portFrom);
      const vistle::Port *vTo = portTo->module()->getVistlePort(portTo);
      m_vistleConnection->disconnect(vFrom, vTo);
   } else {
      m_connections.erase(it);
      removeItem(c);
   }
}

Module *DataFlowNetwork::findModule(int id) const
{
   for (Module *mod: m_moduleList) {
      if (mod->id() == id) {
         return mod;
      }
   }

   return nullptr;
}

Module *DataFlowNetwork::findModule(const boost::uuids::uuid &spawnUuid) const
{
   for (Module *mod: m_moduleList) {
      if (mod->spawnUuid() == spawnUuid) {
         return mod;
      }
   }

   return nullptr;
}

QColor DataFlowNetwork::highlightColor() const {

   return m_highlightColor;
}

///\todo an exception is very occasionally thrown upon a simple click inside a module's port.
///\todo left clicking inside a module's context menu still sends the left click event to the scene, but creates
///a segfault when the connection is completed

/*!
 * \brief Scene::mousePressEvent
 * \param event
 *
 * \todo test connection drawing and unforseen events more thoroughly
 */
void DataFlowNetwork::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    // If the user clicks on a module, test for what is being clicked on.
    //  If okay, begin the process of drawing a line.
    // See what has been selected from an object, if anything.
    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    startPort = dynamic_cast<gui::Port *>(item);
    ///\todo add other objects and dynamic cast checks here
    ///\todo dynamic cast is not a perfect solution, is there a better one?
    if (startPort) {
        // store the click location
        vLastPoint = event->scenePos();
        // set the click flag
        m_mousePressed = true;
        event->accept();

       // Test for port type
       switch (startPort->portType()) {
          case Port::Input:
          case Port::Output:
          case Port::Parameter:
             m_Line = new QGraphicsLineItem(QLineF(event->scenePos(),
                      event->scenePos()));
             m_Line->setPen(QPen(m_LineColor, 2));
             addItem(m_Line);
             startModule = dynamic_cast<Module *>(startPort->parentItem());
             break;
       } //end switch
    } else {
        QGraphicsScene::mousePressEvent(event);
    }
}

/*!
 * \brief Scene::mouseReleaseEvent watches for click events
 * \param event
 */
void DataFlowNetwork::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_mousePressed) {
        QGraphicsScene::mouseReleaseEvent(event);
        return;
    }

    qDebug() << "mouse release";
    // if there was a click
    if (m_Line) {
        qDebug() << "have m_Line";

        // clean up connection
        removeItem(m_Line);
        delete m_Line;
        m_Line = nullptr;

        // Begin testing for the finish of the line draw.
        QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
        if (Port *endPort = dynamic_cast<Port *>(item)) {
            // Test over the port types
            ///\todo improve testing for viability of connection
            auto endModule = dynamic_cast<Module *>(endPort->parentItem());
            assert(endModule);
            switch (endPort->portType()) {
            case Port::Input:
                if (startPort->portType() == Port::Output) {
                    if (startModule != endModule) {
                        addConnection(startPort, endPort, true);
                        qDebug() << "add conn: out -> in";
                    }
                }
                break;
            case Port::Output:
                if (startPort->portType() == Port::Input) {
                    if (startModule != endModule) {
                        addConnection(endPort, startPort, true);
                        qDebug() << "add conn: in -> out";
                    }
                }
                break;
            case Port::Parameter:
                if (startPort->portType() == Port::Parameter) {
                    if (startModule != endModule) {
                        addConnection(startPort, endPort, true);
                        qDebug() << "add conn: par -> par";
                    }
                }
                break;
            }
        }
    }

    // Clear data
    startModule = nullptr;
    startPort = nullptr;
    m_mousePressed = false;
    QGraphicsScene::mouseReleaseEvent(event);
}

/*!
 * \brief Scene::mouseMoveEvent
 * \param event
 */
void DataFlowNetwork::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!startPort  || !m_Line) {
       QGraphicsScene::mouseMoveEvent(event);
       return;
    }

    event->accept();

    // update the line drawing
    QLineF newLine(m_Line->line().p1(), event->scenePos());
    m_Line->setLine(newLine);
}

} //namespace gui
