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
#include "uicontroller.h"
#include "dataflowview.h"
#include "modulebrowser.h"

namespace gui {

const double Module::portDistance = 3.;
boost::uuids::nil_generator nil_uuid;
bool Module::s_snapToGrid = true;
const double Module::borderWidth = 4.;

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
    //setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
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
    delete m_selectUpstreamAct;
    delete m_selectDownstreamAct;
    delete m_createModuleGroup;
}

float Module::gridSpacingX()
{
    return portDistance + Port::portSize;
}

float Module::gridSpacingY()
{
    return gridSpacingX();
}

QPointF Module::gridSpacing()
{
    return QPointF(gridSpacingX(), gridSpacingY());
}

float Module::snapX(float x)
{
    return std::round(x / Module::gridSpacingX()) * Module::gridSpacingX();
}

float Module::snapY(float y)
{
    return std::round(y / Module::gridSpacingY()) * Module::gridSpacingY();
}

QVariant Module::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange) {
        ensureVisible();
    }
    return QGraphicsItem::itemChange(change, value);
}

void Module::projectToGrid()
{
    if (DataFlowView::the()->isSnapToGrid()) {
        auto p = QPointF(snapX(pos().x()), snapY(pos().y()));
        setPos(p);
    }
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
    moveToHub(m_hub);
}

void Module::moveToHub(int hub)
{
    vistle::message::Spawn m(hub, m_name.toStdString());
    m.setMigrateId(m_id);
    vistle::VistleConnection::the().sendMessage(m);
}

void Module::replaceWith(QString mod)
{
    vistle::message::Spawn m(m_hub, mod.toStdString());
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
 */
void Module::createActions()
{
    m_selectUpstreamAct = new QAction("Select Upstream", this);
    m_selectUpstreamAct->setStatusTip("Select all modules feeding data to this one");
    connect(m_selectUpstreamAct, &QAction::triggered, [this]() { emit selectConnected(SelectUpstream, m_id); });

    m_selectDownstreamAct = new QAction("Select Downstream", this);
    m_selectDownstreamAct->setStatusTip("Select all modules this module feeds into");
    connect(m_selectDownstreamAct, &QAction::triggered, [this]() { emit selectConnected(SelectDownstream, m_id); });

    m_selectConnectedAct = new QAction("Select Connected", this);
    m_selectConnectedAct->setStatusTip("Select all modules with a direct connection to this one");
    connect(m_selectConnectedAct, &QAction::triggered, [this]() { emit selectConnected(SelectConnected, m_id); });

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
    m_moduleMenu->addAction(m_selectUpstreamAct);
    m_moduleMenu->addAction(m_selectConnectedAct);
    m_moduleMenu->addAction(m_selectDownstreamAct);
    m_moduleMenu->addSeparator();
    m_moduleMenu->addAction(m_createModuleGroup);
    m_moduleMenu->addAction(m_attachDebugger);
    m_moduleMenu->addAction(m_restartAct);
    m_moveToMenu = m_moduleMenu->addMenu("Move to...");
    m_replaceWithMenu = m_moduleMenu->addMenu("Replace with...");
    m_moduleMenu->addSeparator();
    m_moduleMenu->addAction(m_deleteThisAct);
    m_moduleMenu->addAction(m_deleteSelAct);
}

void Module::doLayout()
{
    prepareGeometryChange();

    // get the pixel width of the string
    QFont font;
    QFontMetrics fm(font);
    QRect nameRect = fm.boundingRect(m_displayName);
    m_fontHeight = nameRect.height() + 4 * portDistance;

    double w = nameRect.width() + 2 * portDistance;
    double h = m_fontHeight + 2 * portDistance;

    QString id = " " + QString::number(m_id);
    QRect idRect = fm.boundingRect(id);

    QRect infoRect;
    if (m_inPorts.isEmpty()) {
        QString t = m_info;
        if (t.length() > 21) {
            t = t.left(20) + "…";
        }
        infoRect = fm.boundingRect(t);
    }

    {
        int idx = 0;
        for (Port *in: m_inPorts) {
            in->setPos(portDistance + idx * (portDistance + Port::portSize), 0.);
            ++idx;
        }
        w = qMax(w, 2 * portDistance + idx * (portDistance + Port::portSize) + infoRect.width() + idRect.width());
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
    const auto m = borderWidth / 2.;
    auto r = rect();
    r = r.marginsAdded(QMarginsF(m, m, m, m));

    return r.united(childrenBoundingRect());
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

    QPen highlightPen(m_borderColor, borderWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    if (isSelected() && m_Status != BUSY) {
        QPen pen(scene()->highlightColor(), borderWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
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
    painter->drawText(QPointF(portDistance, Port::portSize + m_fontHeight / 2.), m_displayName);

    QFont font;
    QFontMetrics fm(font);

    if (m_inPorts.isEmpty()) {
        QString t = m_info;
        if (t.length() > 21) {
            t = t.left(20) + "…";
        }
        painter->drawText(QPointF(portDistance, m_fontHeight / 2.), t);
    }

    QString id = QString::number(m_id);
    QRect idRect = fm.boundingRect(id);
    painter->drawText(rect().x() + rect().width() - idRect.width() - portDistance, m_fontHeight / 2., id);
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
    if (scene() && scene()->moduleBrowser()) {
        auto getModules = [this](int hubId) {
            auto *mb = scene()->moduleBrowser();
            auto hub = mb->getHubItem(m_hub);
            std::vector<QString> modules;
            for (int i = 0; i < hub->childCount(); i++) {
                auto *category = hub->child(i);
                for (int j = 0; j < category->childCount(); ++j) {
                    auto name = category->child(j)->data(0, ModuleBrowser::nameRole()).toString();
                    modules.emplace_back(name);
                }
            }
            return modules;
        };

        static std::vector<std::vector<QString>> replaceables{
            {"COVER", "DisCOVERay", "OsgRenderer", "BlenderRenderer"},
            {"Thicken", "SpheresOld", "TubesOld"},
            {"Threshold", "CellSelect"},
            {"Color", "ColorRandom"},
            {"AddAttribute", "AttachShader", "ColorAttribute", "EnableTransparency", "Variant"},
        };
        m_replaceWithMenu->clear();
        auto *mb = scene()->moduleBrowser();
        auto hub = mb->getHubItem(m_hub);
        if (hub) {
            QString baseName = m_name;
            unsigned ncaps = 0;
            for (int i = 0; i < baseName.size(); ++i) {
                if (baseName.at(i).isUpper()) {
                    ++ncaps;
                    if (ncaps == 3) {
                        baseName = baseName.left(i);
                        break;
                    }
                }
            }
            auto add = [this](QString name) {
                auto act = new QAction(name, this);
                act->setStatusTip(QString("Replace with %1").arg(name));
                connect(act, &QAction::triggered, this, [this, name] { replaceWith(name); });
                m_replaceWithMenu->addAction(act);
            };
            auto modules = getModules(m_hub);
            for (const auto &name: modules) {
                if (name.startsWith(baseName) && m_name != name) {
                    add(name);
                }
            }
            for (const auto &r: replaceables) {
                auto it = std::find(r.begin(), r.end(), m_name);
                if (it != r.end()) {
                    for (const auto &name: modules) {
                        if (name == m_name)
                            continue;
                        for (auto &m: r) {
                            if (m == name) {
                                add(name);
                            }
                        }
                    }
                }
            }
        }

        m_moveToMenu->clear();
        delete m_moveToAct;
        m_moveToAct = nullptr;
        auto hubs = scene()->moduleBrowser()->getHubs();
        unsigned nact = 0;
        int otherHubId = vistle::message::Id::Invalid;
        QString otherHubName;
        for (auto it = hubs.rbegin(); it != hubs.rend(); ++it) {
            const auto &h = *it;
            if (h.first == m_hub)
                continue;
            auto modules = getModules(h.first);
            if (std::find(modules.begin(), modules.end(), m_name) != modules.end()) {
                auto act = new QAction(h.second, this);
                act->setStatusTip(QString("Migrate module to %1 (Id %2)").arg(h.second).arg(h.first));
                int hubId = h.first;
                otherHubId = hubId;
                otherHubName = h.second;
                connect(act, &QAction::triggered, this, [this, hubId] { moveToHub(hubId); });
                m_moveToMenu->addAction(act);
                ++nact;
            }
        }
        if (nact > 1) {
            m_moveToMenu->menuAction()->setVisible(true);
        } else {
            m_moveToMenu->menuAction()->setVisible(false);
            if (otherHubId != vistle::message::Id::Invalid) {
                m_moveToAct = new QAction(QString("Move to %1").arg(otherHubName), this);
                m_moveToAct->setStatusTip(QString("Migrate module to %1 (Id %2)").arg(otherHubName).arg(otherHubId));
                connect(m_moveToAct, &QAction::triggered, this, [this, otherHubId] { moveToHub(otherHubId); });
                m_moduleMenu->insertAction(m_moveToMenu->menuAction(), m_moveToAct);
            }
        }
    } else {
        m_moveToMenu->setVisible(false);
        if (m_moveToAct)
            m_moveToAct->setVisible(false);
    }
    m_replaceWithMenu->menuAction()->setVisible(!m_replaceWithMenu->isEmpty());
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
    Port *guiPort = nullptr;

    switch (port.getType()) {
    case vistle::Port::ANY:
        std::cerr << "cannot handle port type ANY" << std::endl;
        break;
    case vistle::Port::INPUT: {
        guiPort = new Port(&port, this);
        bool update = m_inPorts.isEmpty();
        m_inPorts.push_back(guiPort);
        if (update)
            updateText();
        break;
    }
    case vistle::Port::OUTPUT:
        guiPort = new Port(&port, this);
        m_outPorts.push_back(guiPort);
        break;
    case vistle::Port::PARAMETER:
        //m_paramPorts.push_back(guiPort);
        break;
    }

    if (guiPort)
        m_vistleToGui[port] = guiPort;

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
        if (m_inPorts.isEmpty())
            updateText();
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

QList<Port *> Module::inputPorts() const
{
    return m_inPorts;
}

QList<Port *> Module::outputPorts() const
{
    return m_outPorts;
}

QString Module::name() const
{
    return m_name;
}

void Module::setName(QString name)
{
    m_name = name;
    updateText();
}

void Module::updateText()
{
    //m_displayName = QString("%1_%2").arg(name, QString::number(m_id));
    m_displayName = m_name;
    if (m_inPorts.isEmpty()) {
    } else {
        if (!m_info.isEmpty()) {
            m_displayName = m_name;
            if (m_name == "IndexManifolds")
                m_displayName = "Index";
            if (m_name.startsWith("IsoSurface"))
                m_displayName = "Iso";
            if (m_name.startsWith("CuttingSurface"))
                m_displayName = "Cut";
            if (m_name.startsWith("AddAttribute"))
                m_displayName = "Attr";
            if (m_name.startsWith("Variant"))
                m_displayName = "Var";
            if (m_name.startsWith("Transform"))
                m_displayName = "X";
            if (m_name.startsWith("Thicken"))
                m_displayName = "Th";
            m_displayName += ":" + m_info;
            if (m_displayName.length() > 21) {
                m_displayName = m_displayName.left(20) + "…";
            }
        }
    }

    doLayout();
}

int Module::id() const
{
    return m_id;
}

void Module::setId(int id)
{
    m_id = id;

    doLayout();
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
    projectToGrid();
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
    setCursor(Qt::ClosedHandCursor);
}

void Module::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Base::mouseReleaseEvent(event);
    setCursor(Qt::OpenHandCursor);
    projectToGrid();
    for (auto *item: scene()->selectedItems()) {
        if (auto *mod = dynamic_cast<Module *>(item))
            mod->projectToGrid();
    }
    auto p = getParameter<vistle::ParamVector>("_position");
    if (p) {
        vistle::ParamVector v = p->getValue();
        if (v[0] != pos().x() || v[1] != pos().y()) {
            sendPosition();
            scene()->setSceneRect(scene()->itemsBoundingRect());
        }
    }
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
        if (scene() && scene()->moduleBrowser()) {
            auto *mb = scene()->moduleBrowser();
            auto hub = mb->getHubItem(m_hub);
            if (hub) {
                for (int i = 0; i < hub->childCount(); i++) {
                    auto *category = hub->child(i);
                    for (int j = 0; j < category->childCount(); ++j) {
                        auto name = category->child(j)->data(0, ModuleBrowser::nameRole()).toString();
                        if (name == m_name) {
                            toolTip = category->child(j)->data(0, ModuleBrowser::descriptionRole()).toString();
                        }
                    }
                }
            }
        }
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

    if (!m_info.isEmpty()) {
        toolTip += " - " + m_info;
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

void Module::setInfo(QString text)
{
    m_info = text;
    updateText();
}

} //namespace gui
