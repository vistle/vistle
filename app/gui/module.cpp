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

#include <boost/uuid/nil_generator.hpp>

#include "module.h"
#include "connection.h"
#include "dataflownetwork.h"
#include "mainwindow.h"
#include "uicontroller.h"
#include "dataflowview.h"
#include "modulebrowser.h"

#include <vistle/config/file.h>
#include <vistle/config/array.h>

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
    setLayer(m_layer);

    connect(this, &Module::callshowErrorInMainThread, this, &Module::showError);
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
    if (change == ItemEnabledHasChanged) {
        if (isEnabled()) {
            setToolTip(m_tooltip);
        } else {
            setToolTip("");
        }
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

void Module::sendSpawn(int hub, const std::string &module, vistle::message::Spawn::ReferenceType type)
{
    vistle::message::Spawn m(hub, module);
    m.setReference(m_id, type);
    vistle::VistleConnection::the().sendMessage(m);
}

void Module::cloneModule()
{
    sendSpawn(m_hub, m_name.toStdString(), vistle::message::Spawn::ReferenceType::Clone);
}

void Module::cloneModuleLinked()
{
    sendSpawn(m_hub, m_name.toStdString(), vistle::message::Spawn::ReferenceType::LinkedClone);
}

void Module::moveToHub(int hub)
{
    sendSpawn(hub, m_name.toStdString(), vistle::message::Spawn::ReferenceType::Migrate);
}

void Module::replaceWith(QString mod)
{
    sendSpawn(m_hub, mod.toStdString(), vistle::message::Spawn::ReferenceType::Migrate);
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

void Module::setParameterDefaults()
{
    if (vistle::message::Id::isModule(m_id)) {
        // module id already known
#if 0
        vistle::message::ResetParameters m(m_id);
        m.setDestId(m_id);
        vistle::VistleConnection::the().sendMessage(m);
#endif
    }
}

void Module::showError()
{
    setStatus(m_Status);
    doLayout();
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

    m_createModuleGroup = new QAction("Save As Module Group...", this);
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
    m_restartAct->setStatusTip("Restart the module and keep parameter values");
    connect(m_restartAct, SIGNAL(triggered()), this, SLOT(restartModule()));

    m_cloneModule = new QAction("Clone", this);
    m_cloneModule->setStatusTip("Copy the module with all its parameter values");
    connect(m_cloneModule, &QAction::triggered, this, &Module::cloneModule);

    m_cloneModuleLinked = new QAction("Clone Linked", this);
    m_cloneModuleLinked->setStatusTip("Clone this module with all of its parameters and keep them synced");
    connect(m_cloneModuleLinked, &QAction::triggered, this, &Module::cloneModuleLinked);
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
    m_layerMenu = m_moduleMenu->addMenu("To Layer...");
    m_moduleMenu->addSeparator();
    m_moduleMenu->addAction(m_selectUpstreamAct);
    m_moduleMenu->addAction(m_selectConnectedAct);
    m_moduleMenu->addAction(m_selectDownstreamAct);
    m_moduleMenu->addSeparator();
    m_moduleMenu->addAction(m_cloneModule);
    m_moduleMenu->addAction(m_cloneModuleLinked);
    m_moduleMenu->addAction(m_restartAct);
    m_moveToMenu = m_moduleMenu->addMenu("Move To...");
    m_replaceWithMenu = m_moduleMenu->addMenu("Replace With...");
    m_moduleMenu->addAction(m_attachDebugger);
    m_moduleMenu->addAction(m_createModuleGroup);
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
        if (!m_errorState && m_Status == INITIALIZED) {
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
    auto selected = DataFlowView::the()->selectedModules();
    bool multiSel = isSelected() && selected.size() > 1;
    if (selected.empty())
        selected.push_back(this);
    m_deleteSelAct->setVisible(multiSel);
    m_createModuleGroup->setVisible(multiSel);
    m_deleteThisAct->setVisible(!multiSel);
    m_cloneModule->setVisible(!multiSel);
    m_cloneModuleLinked->setVisible(!multiSel);
    m_restartAct->setVisible(!multiSel);
    m_attachDebugger->setVisible(!multiSel);

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

        static std::vector<std::vector<QString>> replaceables;
        if (replaceables.empty()) {
            // module aliases
            vistle::config::File modules("modules");
            for (auto &group: modules.entries("replace")) {
                if (auto list = modules.array<std::string>("replace", group)) {
                    replaceables.emplace_back();
                    for (auto e: list->value()) {
                        replaceables.back().emplace_back(QString::fromStdString(e));
                    }
                }
            }
        }

        m_replaceWithMenu->clear();
        auto *mb = scene()->moduleBrowser();
        auto hub = mb->getHubItem(m_hub);
        if (hub && selected.size() == 1) {
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
        auto hubs = mb->getHubs();
        unsigned nact = 0;
        int otherHubId = vistle::message::Id::Invalid;
        QString otherHubName;
        for (auto it = hubs.rbegin(); it != hubs.rend(); ++it) {
            const auto &h = *it;
            bool allOnThisHub = true;
            for (auto *m: selected) {
                if (m->hub() != h.first) {
                    allOnThisHub = false;
                    break;
                }
            }
            if (allOnThisHub)
                continue;
            auto modules = getModules(h.first);
            bool allFound = true;
            for (auto *m: selected) {
                if (std::find(modules.begin(), modules.end(), m->name()) == modules.end()) {
                    allFound = false;
                    break;
                }
            }
            std::string what = "module";
            if (selected.size() > 1)
                what = std::to_string(selected.size()) + " modules";
            if (allFound) {
                auto act = new QAction(h.second, this);
                act->setStatusTip(
                    QString("Migrate %3 to %1 (Id %2)").arg(h.second).arg(h.first).arg(QString::fromStdString(what)));
                int hubId = h.first;
                otherHubId = hubId;
                otherHubName = h.second;
                connect(act, &QAction::triggered, this, [this, selected, hubId] {
                    if (isSelected() && selected.size() > 1) {
                        for (auto *m: selected) {
                            if (m->hub() != hubId)
                                m->moveToHub(hubId);
                        }
                    } else {
                        moveToHub(hubId);
                    }
                });
                m_moveToMenu->addAction(act);
                m_moveToAct = act;
                ++nact;
            }
        }
        if (nact > 1) {
            m_moveToMenu->menuAction()->setVisible(true);
        } else {
            m_moveToMenu->menuAction()->setVisible(false);
            if (otherHubId != vistle::message::Id::Invalid) {
                m_moveToAct->setText(
                    QString("Move%2 to %3").arg(selected.size() <= 1 ? "" : " Selected").arg(otherHubName));
                m_moduleMenu->insertAction(m_moveToMenu->menuAction(), m_moveToAct);
            }
        }
    } else {
        m_moveToMenu->setVisible(false);
        if (m_moveToAct)
            m_moveToAct->setVisible(false);
    }
    m_replaceWithMenu->menuAction()->setVisible(!m_replaceWithMenu->isEmpty());

    m_layerMenu->clear();
    auto act = m_layerMenu->addAction("All Layers");
    act->setCheckable(true);
    act->setChecked(layer() == DataFlowNetwork::AllLayers);
    connect(act, &QAction::triggered, [this] {
        if (isSelected() && DataFlowView::the()->selectedModules().size() > 1) {
            for (auto *m: DataFlowView::the()->selectedModules()) {
                m->setLayer(DataFlowNetwork::AllLayers);
            }
        } else {
            setLayer(DataFlowNetwork::AllLayers);
        }
    });
    for (int i = 0; i < DataFlowView::the()->numLayers(); ++i) {
        auto act = m_layerMenu->addAction(QString("Layer %1").arg(i));
        act->setCheckable(true);
        act->setChecked(layer() == i);
        connect(act, &QAction::triggered, [this, i] {
            if (isSelected() && DataFlowView::the()->selectedModules().size() > 1) {
                for (auto *m: DataFlowView::the()->selectedModules()) {
                    m->setLayer(i);
                }
            } else {
                setLayer(i);
            }
        });
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

int Module::layer() const
{
    return m_layer;
}

void Module::setLayer(int layer)
{
    if (m_layer != layer) {
        m_layer = layer;
        setParameter("_layer", vistle::Integer(layer));
    }
    updateLayer();
}

void Module::updateLayer()
{
    bool oldVis = isVisible();
    int activeLayer = DataFlowView::the()->visibleLayer();
    bool newVis =
        m_layer == DataFlowNetwork::AllLayers || m_layer == activeLayer || activeLayer == DataFlowNetwork::AllLayers;

    if (LayersAsOpacity) {
        setEnabled(newVis);
        if (newVis) {
            setOpacity(1.0);
            setAcceptedMouseButtons(Qt::AllButtons);
        } else {
            setOpacity(0.05);
            setAcceptedMouseButtons(Qt::NoButton);
        }
    } else {
        setVisible(newVis);
    }

    if (oldVis != newVis) {
        emit visibleChanged(newVis);
    }
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

    auto items = scene()->selectedItems();
    items.push_back(this);
    for (auto *item: items) {
        if (auto *mod = dynamic_cast<Module *>(item)) {
            mod->setPositionValid();
            auto p = mod->getParameter<vistle::ParamVector>("_position");
            if (p) {
                vistle::ParamVector v = p->getValue();
                if (v[0] != pos().x() || v[1] != pos().y()) {
                    mod->sendPosition();
                }
            }
        }
    }
    scene()->setSceneRect(scene()->itemsBoundingRect());
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
        m_errorState = false;
        m_borderColor = QColor(120, 120, 30);
        break;
    case ERROR_STATUS:
        toolTip = "Error";
        m_borderColor = Qt::red;
        break;
    }

    if (m_errorState) {
        m_borderColor = Qt::red;
    }

    if (!m_info.isEmpty()) {
        toolTip += " - " + m_info;
    }

    m_cancelExecAct->setEnabled(status == BUSY || status == EXECUTING);

    if (m_statusText.isEmpty()) {
        if (isEnabled())
            setToolTip(toolTip);
        m_tooltip = toolTip;
    }

    update();
}

void Module::setStatusText(QString text, int prio)
{
    m_statusText = text;
    if (isEnabled())
        setToolTip(text);
    m_tooltip = text;
    if (text.isEmpty()) {
        setStatus(m_Status);
    }
}

void Module::setInfo(QString text)
{
    m_info = text;
    updateText();
}

void Module::clearMessages()
{
    m_messages.clear();
    m_errorState = false;
    setStatus(m_Status);
    doLayout();
}

void Module::moduleMessage(int type, QString message)
{
    m_messages.push_back({type, message});
    if (type == vistle::message::SendText::Error) {
        if (!m_errorState) {
            m_errorState = true;
            emit callshowErrorInMainThread();
        }
    }
}

QList<Module::Message> &Module::messages()
{
    return m_messages;
}

bool Module::messagesVisible() const
{
    return m_messagesVisible;
}

void Module::setMessagesVisibility(bool visible)
{
    m_messagesVisible = visible;
}

} //namespace gui
