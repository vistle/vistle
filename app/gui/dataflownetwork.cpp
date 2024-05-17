/*********************************************************************************/
/** \file scene.cpp
 *
 * The brunt of the graphical processing and UI interaction happens here --
 * - modules and connections are created and modified
 * - the brunt of the event handling is done
 * - any interactions between modules, such as sorting, is performed
 */
/**********************************************************************************/
#ifdef HAVE_PYTHON
#include <vistle/python/pythoninterface.h>
#include <vistle/python/pythonmodule.h>
#endif

#include "dataflownetwork.h"
#include "connection.h"
#include "vistleconsole.h"
#include "dataflowview.h"
#include "mainwindow.h"

#include <vistle/core/statetracker.h>
#include <vistle/util/directory.h>


#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QtMath>
#if 0
#include <QGuiApplication>
#include <QStyleHints>
#endif

namespace gui {

/*!
 * \brief Scene::Scene
 * \param parent
 */
DataFlowNetwork::DataFlowNetwork(vistle::VistleConnection *conn, MainWindow *mw, QObject *parent)
: QGraphicsScene(parent)
, m_Line(nullptr)
, startPort(nullptr)
, startModule(nullptr)
, endModule(nullptr)
, m_vistleConnection(conn)
, m_state(conn->ui().state())
, m_mainWindow(mw)
{
    // Initialize starting scene information.
    if (isDark()) {
        m_LineColor = Qt::darkGray;
    } else {
        m_LineColor = Qt::black;
    }
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

vistle::StateTracker &DataFlowNetwork::state() const
{
    return m_state;
}

ModuleBrowser *DataFlowNetwork::moduleBrowser() const
{
    if (!m_mainWindow)
        return nullptr;
    return m_mainWindow->moduleBrowser();
}

float abs(const QPointF p)
{
    return qSqrt(p.x() * p.x() + p.y() * p.y());
}

void DataFlowNetwork::addModule(int hub, QString modName, Qt::Key direction)
{
    QPointF offset;
    switch (direction) {
    case Qt::Key_Down:
        offset = QPointF{0, 80};
        break;
    case Qt::Key_Up:
        offset = QPointF{0, -80};
        break;
    case Qt::Key_Left:
        offset = QPointF{-160, 0};
        break;
    case Qt::Key_Right:
        offset = QPointF{160, 0};
        break;

    default:
        break;
    }
    QPointF newPos = lastDropPos + offset;
    constexpr float threashhold = 30;
    for (int i = 0; i < m_moduleList.size(); i++) {
        if (abs(m_moduleList[i]->pos() - newPos) < threashhold) {
            newPos += offset;
            i = -1;
        }
    }

    addModule(hub, modName, newPos);
}

Module *DataFlowNetwork::newModule(QString modName)
{
    Module *module = new Module(nullptr, modName);
    addItem(module);
    m_moduleList.append(module);
    module->setStatus(Module::SPAWNING);
    connect(module, &Module::createModuleCompound, this, &DataFlowNetwork::createModuleCompound);
    connect(module, &Module::selectConnected, this, &DataFlowNetwork::selectConnected);
    connect(module, &Module::visibleChanged, this, &DataFlowNetwork::updateConnectionVisibility);
    return module;
}


/*!
 * \brief Scene::addModule add a module to the draw area.
 * \param modName
 * \param dropPos
 */
void DataFlowNetwork::addModule(int hub, QString modName, QPointF dropPos)
{
    auto module = newModule(modName);
    lastDropPos = dropPos;
    ///\todo improve how the data such as the name is set in the module.
    module->setLayer(DataFlowView::the()->visibleLayer());
    module->setPos(dropPos);
    module->setPositionValid();
    vistle::message::Spawn spawnMsg(hub, modName.toUtf8().constData());
    spawnMsg.setDestId(vistle::message::Id::MasterHub); // to master, for module id generation
    module->setSpawnUuid(spawnMsg.uuid());
    m_vistleConnection->sendMessage(spawnMsg);

    ///\todo add the objects only to the map (sortMap) currently used for sorting, not to the list.
    ///This will remove the need for moduleList altogether
    module->setHub(hub);
}

void DataFlowNetwork::addModule(int moduleId, const boost::uuids::uuid &spawnUuid, QString name)
{
    if (!vistle::message::Id::isModule(moduleId))
        return;

    //std::cerr << "addModule: name=" << name.toStdString() << ", id=" << moduleId << std::endl;
    Module *mod = findModule(spawnUuid);
    if (mod) {
        mod->sendPosition();
    } else {
        mod = findModule(moduleId);
    }
    if (!mod) {
        mod = newModule(name);
    }

    mod->setId(moduleId);
    mod->setHub(m_state.getHub(moduleId));
    mod->setName(name);
}

void DataFlowNetwork::deleteModule(int moduleId)
{
    Module *m = findModule(moduleId);
    if (m) {
        std::vector<Port *> p1, p2;
        for (auto &c: m_connections) {
            const auto &key = c.first;
            if (key.port1->module()->id() == moduleId || key.port2->module()->id() == moduleId) {
                p1.push_back(key.port1);
                p2.push_back(key.port2);
            }
        }
        for (size_t i = 0; i < p1.size(); ++i) {
            removeConnection(p1[i], p2[i]);
        }

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
        else if (stateBits & vistle::StateObserver::Executing)
            m->setStatus(Module::EXECUTING);
        else if (stateBits & vistle::StateObserver::Initialized)
            m->setStatus(Module::INITIALIZED);
        else
            m->setStatus(Module::SPAWNING);
    }
}

void DataFlowNetwork::newPort(int moduleId, QString portName)
{
    if (Module *m = findModule(moduleId)) {
        std::lock_guard guard(m_state);
        const vistle::Port *port = m_state.portTracker()->getPort(moduleId, portName.toStdString());
        if (port) {
            m->addPort(*port);
        }
    }
}

void DataFlowNetwork::deletePort(int moduleId, QString portName)
{
#if 0
   QString text = "Delete port: " + QString::number(moduleId) + ":" + portName;
   VistleConsole::the()->appendDebug(text);
   std::cerr << text.toStdString() << std::endl;
#endif
    if (Module *m = findModule(moduleId)) {
        std::vector<Port *> p1, p2;
        for (auto &c: m_connections) {
            const auto &key = c.first;
            if ((key.port1->module()->id() == moduleId && key.port1->name() == portName) ||
                (key.port2->module()->id() == moduleId && key.port1->name() == portName)) {
                p1.push_back(key.port1);
                p2.push_back(key.port2);
            }
        }
        for (size_t i = 0; i < p1.size(); ++i) {
            removeConnection(p1[i], p2[i]);
        }
        m->removePort(vistle::Port(moduleId, portName.toStdString(), vistle::Port::ANY));
    }
}

void DataFlowNetwork::newConnection(int fromId, QString fromName, int toId, QString toName)
{
#if 0
   QString text = "New Connection: " + QString::number(fromId) + ":" + fromName + " -> " + QString::number(toId) + ":" + toName;
   m_console->appendDebug(text);
#endif

    std::lock_guard guard(m_state);
    const vistle::Port *portFrom = m_state.portTracker()->findPort(fromId, fromName.toStdString());
    const vistle::Port *portTo = m_state.portTracker()->findPort(toId, toName.toStdString());

    Module *mFrom = findModule(fromId);
    Module *mTo = findModule(toId);

    if (mFrom && portFrom && mTo && portTo) {
        addConnection(mFrom->getGuiPort(portFrom), mTo->getGuiPort(portTo));
    }
}

void DataFlowNetwork::deleteConnection(int fromId, QString fromName, int toId, QString toName)
{
#if 0
   QString text = "Connection removed: " + QString::number(fromId) + ":" + fromName + " -> " + QString::number(toId) + ":" + toName;
   m_console->appendDebug(text);
#endif

    std::lock_guard guard(m_state);

    const vistle::Port *portFrom = m_state.portTracker()->findPort(fromId, fromName.toStdString());
    const vistle::Port *portTo = m_state.portTracker()->findPort(toId, toName.toStdString());

    Module *mFrom = findModule(fromId);
    Module *mTo = findModule(toId);

    if (mFrom && portFrom && mTo && portTo) {
        auto guiFrom = mFrom->getGuiPort(portFrom);
        auto guiTo = mTo->getGuiPort(portTo);
        removeConnection(guiFrom, guiTo);
    } //else {
    //     auto conn = std::find_if(
    //         m_connections.begin(), m_connections.end(),
    //         [fromId, &fromName, toId, &toName](const std::pair<ConnectionKey, Connection *> &conn) {
    //             std::cerr << "comparing: \nconn.first.port1->module()->id() = " << conn.first.port1->module()->id()
    //                       << "\nconn.first.port1->vistlePort()->getName() = "
    //                       << conn.first.port1->vistlePort()->getName()
    //                       << "\nconn.first.port2->module()->id() = " << conn.first.port2->module()->id()
    //                       << "\nconn.first.port2->vistlePort()->getName()" << conn.first.port2->vistlePort()->getName()
    //                       << std::endl;

    //             return (conn.first.port1->module()->id() == fromId && conn.first.port2->module()->id() == toId &&
    //                     conn.first.port1->vistlePort()->getName() == fromName.toStdString() &&
    //                     conn.first.port2->vistlePort()->getName() == toName.toStdString()) ||
    //                    (conn.first.port1->module()->id() == toId && conn.first.port2->module()->id() == fromId &&
    //                     conn.first.port1->vistlePort()->getName() == toName.toStdString() &&
    //                     conn.first.port2->vistlePort()->getName() == fromName.toStdString());
    //         });
    //     if (conn != m_connections.end()) {
    //         auto c = conn->second;
    //         m_connections.erase(conn);
    //         removeItem(c);
    //     }
    // }
}

void DataFlowNetwork::moduleStatus(int id, QString status, int prio)
{
    if (Module *m = findModule(id)) {
        m->setStatusText(status, prio);
    }
}

void DataFlowNetwork::itemInfoChanged(QString text, int type, int id, QString port)
{
    if (Module *m = findModule(id)) {
        if (port.isEmpty()) {
            m->setInfo(text);
        } else {
            std::lock_guard guard(m_state);
            const vistle::Port *p = m_state.portTracker()->findPort(id, port.toStdString());
            if (auto *gp = m->getGuiPort(p)) {
                gp->setInfo(text);
            }
        }
    }
}

void DataFlowNetwork::moduleMessage(int senderId, int type, QString message)
{
    if (Module *m = findModule(senderId)) {
        m->moduleMessage(type, message);
    }
}

void DataFlowNetwork::clearMessages(int moduleId)
{
    if (Module *m = findModule(moduleId)) {
        m->clearMessages();
    }
}

void DataFlowNetwork::messagesVisibilityChanged(int moduleId, bool visible)
{
    if (Module *m = findModule(moduleId)) {
        m->setMessagesVisibility(visible);
    }
}


void DataFlowNetwork::addConnection(Port *portFrom, Port *portTo, bool sendToController)
{
    if (!portFrom) {
        // parameter ports currently do not have a GUI representation
        return;
    }
    if (!portTo) {
        return;
    }

    assert(portFrom);
    assert(portTo);

    ConnectionKey key(portFrom, portTo);
    auto it = m_connections.end();
    if (key.valid())
        it = m_connections.find(key);
    Connection *c = nullptr;
    if (it != m_connections.end()) {
        //qDebug() << "already have connection";
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

    c->updateVisibility(DataFlowView::the()->visibleLayer());
    portFrom->update();
    portTo->update();
}

void DataFlowNetwork::removeConnection(Port *portFrom, Port *portTo, bool sendToController)
{
    if (!portFrom)
        return;
    if (!portTo)
        return;

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
        const vistle::Port *vFrom = portFrom->vistlePort();
        const vistle::Port *vTo = portTo->vistlePort();
        if (vFrom && vTo)
            m_vistleConnection->disconnect(vFrom, vTo);
    } else {
        m_connections.erase(it);
        removeItem(c);
    }

    portFrom->update();
    portTo->update();
}

void DataFlowNetwork::setConnectionHighlights(Port *port, bool highlight)
{
    if (!port)
        return;

    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        Connection *c = it->second;
        if (c->source() != port && c->destination() != port)
            continue;

        c->setHighlight(highlight);
    }
}

void DataFlowNetwork::removeConnections(Port *port, bool sendToController)
{
    if (!port)
        return;

    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        Connection *c = it->second;
        if (c->source() != port && c->destination() != port)
            continue;

        if (sendToController) {
            c->setState(Connection::ToRemove);
            const vistle::Port *vFrom = c->source()->vistlePort();
            const vistle::Port *vTo = c->destination()->vistlePort();
            if (vFrom && vTo)
                m_vistleConnection->disconnect(vFrom, vTo);
        } else {
            m_connections.erase(it);
            removeItem(c);
        }
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

bool DataFlowNetwork::moveModule(int moduleId, float x, float y)
{
    if (Module *m = findModule(moduleId)) {
        m->setPos(x, y);
        m->setPositionValid();
        lastDropPos = {x, y};
        return true;
    }
    return false;
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

bool DataFlowNetwork::isDark() const
{
#if 0
    return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    const auto &palette = DataFlowView::the()->palette();
    auto &fg = palette.color(QPalette::Text);
    auto &bg = palette.color(QPalette::Base);
    return fg.value() > bg.value();
#endif
}

QColor DataFlowNetwork::highlightColor() const
{
    return m_highlightColor;
}


///\todo an exception is very occasionally thrown upon a simple click inside a module's port.
///\todo left clicking inside a module's context menu still sends the left click event to the scene, but creates
///a segfault when the connection is completed

/*!
 * \brief Scene::mousePressEvent
 * \param event
 *
 * \todo test connection drawing and unforeseen events more thoroughly
 */
void DataFlowNetwork::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        //QGraphicsScene::mousePressEvent(event);
        event->accept(); // accepting instead of forwarding the event retains current selection when right-clicking
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
            m_Line = new QGraphicsLineItem(QLineF(event->scenePos(), event->scenePos()));
            m_Line->setPen(QPen(m_LineColor, 3));
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

    //qDebug() << "mouse release";
    // if there was a click
    if (m_Line) {
        //qDebug() << "have m_Line";

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
                        //qDebug() << "add conn: out -> in";
                    }
                }
                break;
            case Port::Output:
                if (startPort->portType() == Port::Input) {
                    if (startModule != endModule) {
                        addConnection(endPort, startPort, true);
                        //qDebug() << "add conn: in -> out";
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
    if (!startPort || !m_Line) {
        QGraphicsScene::mouseMoveEvent(event);
        return;
    }

    event->accept();

    // update the line drawing
    QLineF newLine(m_Line->line().p1(), event->scenePos());
    m_Line->setLine(newLine);
}

void DataFlowNetwork::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    // especially ignore wheel events on disabled items
    event->ignore();
}

void DataFlowNetwork::selectConnected(int direction, int id, QString port)
{
    std::lock_guard guard(m_state);
    std::set<int> modules;
    switch (direction) {
    case SelectConnected:
        modules = m_state.getConnectedModules(vistle::StateTracker::Neighbor, id, port.toStdString());
        break;
    case SelectUp:
        modules = m_state.getConnectedModules(vistle::StateTracker::Previous, id, port.toStdString());
        break;
    case SelectUpstream:
        modules = m_state.getConnectedModules(vistle::StateTracker::Upstream, id, port.toStdString());
        break;
    case SelectDown:
        modules = m_state.getConnectedModules(vistle::StateTracker::Next, id, port.toStdString());
        break;
    case SelectDownstream:
        modules = m_state.getConnectedModules(vistle::StateTracker::Downstream, id, port.toStdString());
        break;
    }
    selectModules(modules);
}

void DataFlowNetwork::selectModules(const std::set<int> &moduleIds)
{
    for (auto *m: m_moduleList) {
        if (std::find(moduleIds.begin(), moduleIds.end(), m->id()) != moduleIds.end()) {
            m->setSelected(true);
        }
    }
}

void DataFlowNetwork::emphasizeConnections(QList<Module *> modules)
{
    for (auto &c: m_connections) {
        auto &key = c.first;
        auto *conn = c.second;
        auto *m1 = key.port1->module();
        auto *m2 = key.port2->module();
        conn->setEmphasis(modules.contains(m1) || modules.contains(m2));
    }
}

QRectF DataFlowNetwork::computeBoundingRect(int layer) const
{
    QRectF bounds;
    for (auto *m: m_moduleList) {
        if (layer == AllLayers || m->layer() == AllLayers || layer == m->layer()) {
            bounds = bounds.united(m->mapToScene(m->boundingRect()).boundingRect());
        }
    }
    return bounds;
}

void DataFlowNetwork::updateConnectionVisibility()
{
    int layer = DataFlowView::the()->visibleLayer();
    for (auto &c: m_connections) {
        c.second->updateVisibility(layer);
    }
}

void DataFlowNetwork::visibleLayerChanged(int layer)
{
    for (auto *m: m_moduleList) {
        m->updateLayer();
    }
    updateConnectionVisibility();
}

void DataFlowNetwork::createModuleCompound()
{
    std::cerr << "saving preset" << std::endl;
    QString filter = "Vistle Compound Files (*";
    filter += vistle::moduleCompoundSuffix.c_str();
    filter += +")";
    QString text = QFileDialog::getSaveFileName(nullptr, tr("Save Compound"), vistle::directory::configHome().c_str(),
                                                filter, &filter);
    if (text.size() - text.lastIndexOf("/") > vistle::ModuleNameLength) {
#ifdef HAVE_PYTHON
        vistle::PythonInterface::the().exec("print(\"Module preset must not exceed " +
                                            std::to_string(vistle::ModuleNameLength) + "!\"");
#endif
        return;
    }

    auto path = text.toStdString();
    auto name = path.substr(path.find_last_of("/") + 1);
    vistle::ModuleCompound comp{0, name, path, "Compound", ""};
    auto selectedModules = DataFlowView::the()->selectedModules();
    std::sort(selectedModules.begin(), selectedModules.end(),
              [](const gui::Module *m1, const gui::Module *m2) { return m1->id() < m2->id(); });

    int i = 0;
    for (const auto &m: selectedModules) {
        if (m->hub() != selectedModules[0]->hub()) {
#ifdef HAVE_PYTHON
            vistle::PythonInterface::the().exec("print(\"Modules in a module preset must be on a single host!\"");
            return;
#endif
        }
        comp.addSubmodule(vistle::ModuleCompound::SubModule{
            m->name().toStdString(), static_cast<float>(m->pos().x() - selectedModules[0]->pos().x()),
            static_cast<float>(m->pos().y() - selectedModules[0]->pos().y())});

        std::lock_guard guard(m_state);
        for (const auto &fromPort: m_state.portTracker()->getConnectedOutputPorts(m->id())) {
            for (const auto &toPort: fromPort->connections()) {
                vistle::ModuleCompound::Connection c;
                c.fromId = i;
                auto toMod = std::find_if(selectedModules.begin(), selectedModules.end(),
                                          [&toPort](const Module *mod) { return toPort->getModuleID() == mod->id(); });

                c.toId = toMod == selectedModules.end()
                             ? -1
                             : toMod - selectedModules.begin(); //-1 for an exposed output port for the compound
                c.fromPort = fromPort->getName();
                c.toPort = toPort->getName();
                comp.addConnection(c);
            }
        }
        //get exposed input ports

        for (const auto &toPort: m_state.portTracker()->getConnectedInputPorts(m->id())) {
            for (const auto &fromPort: toPort->connections()) {
                auto mod = std::find_if(selectedModules.begin(), selectedModules.end(), [&fromPort](const Module *mod) {
                    return mod->id() == fromPort->getModuleID();
                });
                if (mod == selectedModules.end()) {
                    vistle::ModuleCompound::Connection c;
                    c.fromId = -1;
                    c.toId = i;
                    c.fromPort = fromPort->getName();
                    c.toPort = toPort->getName();
                    comp.addConnection(c);
                }
            }
        }
        ++i;
    }
    comp.send(std::bind(&vistle::VistleConnection::sendMessage, m_vistleConnection, std::placeholders::_1,
                        std::placeholders::_2));
}


} //namespace gui
