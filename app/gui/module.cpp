/*********************************************************************************/
/*! \file module.cpp
 *
 * representation, both graphical and of information, for vistle modules.
 */
/**********************************************************************************/


#include <cassert>

#include <QDebug>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QInputDialog>
#include <QMenu>
#include <QDir>

#include <vistle/python/pythonmodule.h>
#include <vistle/userinterface/vistleconnection.h>
#include <vistle/util/directory.h>

#include <boost/uuid/nil_generator.hpp>

#include "module.h"
#include "connection.h"
#include "dataflownetwork.h"
#include "mainwindow.h"
#include "dataflowview.h"

namespace gui {

const double Module::portDistance = 3.;
boost::uuids::nil_generator nil_uuid;

/*!
 * \brief Module::Module
 * \param parent
 * \param name
 *
 * \todo move the generation of the ports and the main shape out of the constructor
 */
Module::Module(QGraphicsItem *parent, QString name)
: Base(parent)
, m_hub(0)
, m_id(vistle::message::Id::Invalid)
, m_spawnUuid(nil_uuid())
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
    delete m_attachDebugger;
    delete m_cancelExecAct;
    delete m_deleteThisAct;
    delete m_deleteSelAct;
    delete m_createModuleGroup;
}

void Module::execModule()
{
    vistle::message::Execute m(vistle::message::Execute::ComputeExecute, m_id);
    vistle::VistleConnection::the().sendMessage(m);
}

void Module::cancelExecModule()
{
    vistle::message::CancelExecute m(m_id);
    m.setDestId(m_id);
    vistle::VistleConnection::the().sendMessage(m);
}

void Module::restartModule()
{
    vistle::message::Spawn m(m_hub, m_name.toStdString());
    m.setMigrateId(m_id);
    vistle::VistleConnection::the().sendMessage(m);
}

/*!
 * \brief Module::deleteModule
 */
void Module::deleteModule()
{
    setStatus(KILLED);

    cancelExecModule();

    if (vistle::message::Id::isModule(m_id)) {
        // module id already known
        vistle::message::Kill m(m_id);
        m.setDestId(m_id);
        vistle::VistleConnection::the().sendMessage(m);
    }
}

void Module::attachDebugger()
{
    vistle::message::Debug m(m_id);
    m.setDestId(m_hub);
    vistle::VistleConnection::the().sendMessage(m);
}

void Module::createGeometry()
{
    setHub(m_hub);
}

/*!
 * \brief Module::createActions
 *
 * \todo this doesn't really work at the moment, find out what is wrong
 */
void Module::createActions()
{
    m_deleteThisAct = new QAction("Delete", this);
    m_deleteThisAct->setShortcuts(QKeySequence::Delete);
    m_deleteThisAct->setStatusTip("Delete the module and all of its connections");
    connect(m_deleteThisAct, SIGNAL(triggered()), this, SLOT(deleteModule()));

    m_deleteSelAct = new QAction("Delete Selected", this);
    m_deleteSelAct->setShortcuts(QKeySequence::Delete);
    m_deleteSelAct->setStatusTip("Delete the selected modules and all their connections");
    connect(m_deleteSelAct, SIGNAL(triggered()), DataFlowView::the(), SLOT(deleteModules()));

    m_createModuleGroup = new QAction("Save as module group", this);
    m_createModuleGroup->setStatusTip("Save a named preset of the selected modules");
    connect(m_createModuleGroup, &QAction::triggered, this, &Module::createModuleCompound);

    m_execAct = new QAction("Execute", this);
    m_execAct->setStatusTip("Execute the module");
    connect(m_execAct, SIGNAL(triggered()), this, SLOT(execModule()));

    m_attachDebugger = new QAction("Attach Debugger", this);
    m_attachDebugger->setStatusTip("Debug running module");
    connect(m_attachDebugger, SIGNAL(triggered(bool)), this, SLOT(attachDebugger()));

    m_cancelExecAct = new QAction("Cancel Execution", this);
    m_cancelExecAct->setStatusTip("Interrupt execution of module");
    connect(m_cancelExecAct, SIGNAL(triggered()), this, SLOT(cancelExecModule()));

    m_restartAct = new QAction("Restart", this);
    m_restartAct->setStatusTip("Restart the module");
    connect(m_restartAct, SIGNAL(triggered()), this, SLOT(restartModule()));
}

/*!
 * \brief Module::createMenus
 */
void Module::createMenus()
{
    m_moduleMenu = new QMenu();
    m_moduleMenu->addAction(m_execAct);
    m_moduleMenu->addAction(m_cancelExecAct);
    m_moduleMenu->addSeparator();
    m_moduleMenu->addAction(m_attachDebugger);
    m_moduleMenu->addAction(m_restartAct);
    m_moduleMenu->addAction(m_deleteThisAct);
    m_moduleMenu->addAction(m_deleteSelAct);
    m_moduleMenu->addAction(m_createModuleGroup);
}

void Module::doLayout()
{
    prepareGeometryChange();

    // get the pixel width of the string
    QFont font;
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(m_displayName);
    m_fontHeight = textRect.height() + 4 * portDistance;

    double w = textRect.width() + 2 * portDistance;
    double h = m_fontHeight + 2 * portDistance;

    {
        int idx = 0;
        for (Port *in: m_inPorts) {
            in->setPos(portDistance + idx * (portDistance + Port::portSize), 0.);
            ++idx;
        }
        w = qMax(w, 2 * portDistance + idx * (portDistance + Port::portSize));
    }

    {
        int idx = 0;
        for (Port *out: m_outPorts) {
            out->setPos(portDistance + idx * (portDistance + Port::portSize), h);
            ++idx;
        }
        w = qMax(w, 2 * portDistance + idx * (portDistance + Port::portSize));
    }

    {
        int idx = 0;
        for (Port *param: m_paramPorts) {
            param->setPos(portDistance + idx * (portDistance + Port::portSize), 0);
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
QRectF Module::boundingRect() const
{
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
    painter->drawText(QPointF(portDistance, portDistance + Port::portSize + m_fontHeight / 2.), m_displayName);
}

/*!
 * \brief Module::contextMenuEvent
 * \param event
 */
void Module::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (isSelected() && DataFlowView::the()->selectedModules().size() > 1) {
        m_deleteSelAct->setVisible(true);
        m_createModuleGroup->setVisible(true);
        m_deleteThisAct->setVisible(false);
    } else {
        m_deleteSelAct->setVisible(false);
        m_createModuleGroup->setVisible(false);
        m_deleteThisAct->setVisible(true);
    }
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

void Module::addPort(const vistle::Port &port)
{
    Port *guiPort = new Port(&port, this);
    m_vistleToGui[port] = guiPort;

    switch (port.getType()) {
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

void Module::removePort(const vistle::Port &port)
{
    auto it = m_vistleToGui.find(port);
    //std::cerr << "removePort(" << port << "): found: " << (it!=m_vistleToGui.end()) << std::endl;
    if (it == m_vistleToGui.end())
        return;
    auto gport = it->second;
    auto vport = gport->vistlePort();
    if (!vport)
        return;

    switch (vport->getType()) {
    case vistle::Port::ANY:
        std::cerr << "cannot handle port type ANY" << std::endl;
        break;
    case vistle::Port::INPUT: {
        auto it = std::find(m_inPorts.begin(), m_inPorts.end(), gport);
        if (it != m_inPorts.end())
            m_inPorts.erase(it);
        break;
    }
    case vistle::Port::OUTPUT: {
        auto it = std::find(m_outPorts.begin(), m_outPorts.end(), gport);
        if (it != m_outPorts.end())
            m_outPorts.erase(it);
        break;
    }
    case vistle::Port::PARAMETER: {
        auto it = std::find(m_paramPorts.begin(), m_paramPorts.end(), gport);
        if (it != m_paramPorts.end())
            m_paramPorts.erase(it);
        break;
    }
    }

    m_vistleToGui.erase(it);
    delete gport;

    doLayout();
}

QString Module::name() const
{
    return m_name;
}

void Module::setName(QString name)
{
    m_name = name;
    m_displayName = QString("%1_%2").arg(name, QString::number(m_id));

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

int Module::hub() const
{
    return m_hub;
}

void Module::setHub(int hub)
{
    m_hub = hub;
    m_color = hubColor(hub);
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

bool Module::isPositionValid() const
{
    return m_validPosition;
}

void Module::setPositionValid()
{
    m_validPosition = true;
}

Port *Module::getGuiPort(const vistle::Port *port) const
{
    const auto it = m_vistleToGui.find(*port);
    if (it == m_vistleToGui.end())
        return nullptr;

    return it->second;
}

const vistle::Port *Module::getVistlePort(Port *port) const
{
    return port->vistlePort();
}

DataFlowNetwork *Module::scene() const
{
    return static_cast<DataFlowNetwork *>(Base::scene());
}

QColor Module::hubColor(int hub)
{
    int h = std::abs(hub - vistle::message::Id::MasterHub);
    const int r = h % 2;
    const int g = 1 - (h >> 2) % 2;
    const int b = 1 - (h >> 1) % 2;
    return QColor(100 + r * 100, 100 + g * 100, 100 + b * 100);
}

void Module::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Base::mousePressEvent(event);
}

void Module::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    auto p = getParameter<vistle::ParamVector>("_position");
    if (p) {
        vistle::ParamVector v = p->getValue();
        if (v[0] != pos().x() || v[1] != pos().y())
            sendPosition();
    }
    Base::mouseReleaseEvent(event);
}

void Module::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    execModule();
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
            return QPointF(portDistance + idx * (Port::portSize + portDistance) + 0.5 * Port::portSize,
                           0.0 * Port::portSize);
        }
    }

    {
        int idx = m_outPorts.indexOf(const_cast<Port *>(port));
        if (idx >= 0) {
            return QPointF(portDistance + idx * (Port::portSize + portDistance) + 0.5 * Port::portSize,
                           rect().bottom() - 0.0 * Port::portSize);
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
    case EXECUTING:
        toolTip = "Executing";
        m_borderColor = QColor(120, 120, 30);
        break;
    case ERROR_STATUS:
        toolTip = "Error";
        m_borderColor = Qt::red;
        break;
    }

    m_cancelExecAct->setEnabled(status == BUSY || status == EXECUTING);

    if (m_statusText.isEmpty()) {
        setToolTip(toolTip);
    }

    update();
}

void Module::setStatusText(QString text, int prio)
{
    m_statusText = text;
    setToolTip(text);
    if (text.isEmpty()) {
        setStatus(m_Status);
    }
}

} //namespace gui
