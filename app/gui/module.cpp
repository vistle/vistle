/*********************************************************************************/
/*! \file module.cpp
 *
 * representation, both graphical and of information, for vistle modules.
 */
/**********************************************************************************/


#include <cassert>

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
#include "errorindicator.h"
#include "colormap.h"
#include "dataflownetwork.h"
#include "mainwindow.h"
#include "uicontroller.h"
#include "dataflowview.h"
#include "modulebrowser.h"
#include "parameterconnectionwidgets.h"
#include "ui_setnamedialog.h"

#include <vistle/config/file.h>
#include <vistle/config/array.h>
#include <vistle/config/value.h>

namespace gui {

boost::uuids::nil_generator nil_uuid;
double Module::portDistance = 3.;
double Module::borderWidth = 4.;

void Module::configure()
{
    vistle::config::File config("gui");
    portDistance = *config.value<double>("module", "port_spacing", 3.);
    borderWidth = *config.value<double>("module", "border_width", 4.);
}

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
, m_errorIndicator(new ErrorIndicator(this))
, m_fontHeight(0.)
{
    setName(name);

    m_errorIndicator->setVisible(false);

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    //setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setCursor(Qt::OpenHandCursor);
    createParameterPopup();
    createActions();
    createMenus();
    createGeometry();

    setStatus(m_Status);
    setLayer(m_layer);

    connect(this, &Module::callshowErrorInMainThread, this, &Module::showError);
    connect(m_errorIndicator, &ErrorIndicator::clicked, this, &Module::showError);
}

Module::~Module()
{
    delete m_moduleMenu;
    delete m_execAct;
    delete m_attachDebugger;
    delete m_replayOutput;
    delete m_toggleOutputStreaming;
    delete m_cancelExecAct;
    delete m_deleteThisAct;
    delete m_deleteSelAct;
    delete m_selectUpstreamAct;
    delete m_selectDownstreamAct;
    delete m_createModuleGroup;
    delete m_parameterPopup;
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

    if (change == ItemSelectedChange && value.toBool() && scene() && scene()->moduleBrowser()) {
        scene()->moduleBrowser()->clearFocus(); // the focus prevents key events to reach the dataflowView
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

void Module::highlightModule(int moduleId)
{
    if (moduleId == m_id) {
        m_highlighted = true;
    } else if (moduleId == -1) {
        m_highlighted = false;
    }
    update();
}

void Module::attachDebugger()
{
    vistle::message::Debug m(m_id, vistle::message::Debug::AttachDebugger);
    m.setDestId(m_hub);
    vistle::VistleConnection::the().sendMessage(m);
}

void Module::replayOutput()
{
    vistle::message::Debug m(m_id, vistle::message::Debug::ReplayOutput);
    m.setDestId(m_hub);
    vistle::VistleConnection::the().sendMessage(m);
}

void Module::setOutputStreaming(bool enable)
{
    using vistle::message::Debug;

    vistle::message::Debug m(m_id, Debug::SwitchOutputStreaming,
                             enable ? Debug::SwitchAction::SwitchOn : Debug::SwitchAction::SwitchOff);
    m.setDestId(m_hub);
    vistle::VistleConnection::the().sendMessage(m);

    m_toggleOutputStreaming->setChecked(enable);
    emit outputStreamingChanged(enable);
}

bool Module::isOutputStreaming() const
{
    return m_toggleOutputStreaming->isChecked();
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
    m_changeNameAct = new QAction("Rename", this);
    m_changeNameAct->setStatusTip("Change the display name of this module");
    connect(m_changeNameAct, &QAction::triggered, [this]() { emit changeDisplayName(); });

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

    m_replayOutput = new QAction("Replay Output", this);
    m_replayOutput->setStatusTip("Send last lines of console output to GUI");
    connect(m_replayOutput, SIGNAL(triggered(bool)), this, SLOT(replayOutput()));

    m_toggleOutputStreaming = new QAction("Stream Output", this);
    m_toggleOutputStreaming->setCheckable(true);
    m_toggleOutputStreaming->setStatusTip("Switch streaming of console output on or off");
    connect(m_toggleOutputStreaming, SIGNAL(triggered(bool)), this, SLOT(setOutputStreaming(bool)));

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

void Module::changeDisplayName()
{
    QDialog *dialog = new QDialog;
    ::Ui::SetNameDialog *ui = new ::Ui::SetNameDialog;
    ui->setupUi(dialog);
    connect(ui->buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()), ui->lineEdit, SLOT(clear()));
    dialog->setFocusProxy(ui->lineEdit);
    ui->lineEdit->setFocus();

    ui->lineEdit->setText(m_displayName);
    dialog->exec();

    switch (dialog->result()) {
    case QDialog::Rejected:
        return;

    case QDialog::Accepted:
        break;
    }

    QString name = ui->lineEdit->text();
    vistle::message::SetName m(m_id, name.toStdString());
    vistle::VistleConnection::the().sendMessage(m);
}

/*!
 * \brief Module::createMenus
 */
void Module::createMenus()
{
    m_moduleMenu = new QMenu();
    m_moduleMenu->addAction(m_execAct);
    m_moduleMenu->addAction(m_cancelExecAct);
    m_moduleMenu->addAction(m_changeNameAct);
    m_moduleMenu->addSeparator();
    m_layerMenu = m_moduleMenu->addMenu("To Layer...");
    m_moduleMenu->addSeparator();
    m_moduleMenu->addAction(m_selectUpstreamAct);
    m_moduleMenu->addAction(m_selectConnectedAct);
    m_moduleMenu->addAction(m_selectDownstreamAct);
    m_moduleMenu->addSeparator();
    m_moduleMenu->addAction(m_cloneModule);
    m_moduleMenu->addAction(m_cloneModuleLinked);
    m_moveToMenu = m_moduleMenu->addMenu("Move To...");
    m_replaceWithMenu = m_moduleMenu->addMenu("Replace With...");
    m_advancedMenu = m_moduleMenu->addMenu("Advanced...");
    m_advancedMenu->addAction(m_replayOutput);
    m_advancedMenu->addAction(m_toggleOutputStreaming);
    m_advancedMenu->addAction(m_restartAct);
    m_advancedMenu->addAction(m_attachDebugger);
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
    QRect nameRect = fm.boundingRect(m_visibleName);
    m_fontHeight = nameRect.height() + 4 * portDistance;

    QRect errorRect;
    if (m_errorIndicator->isVisible()) {
        errorRect.setWidth(m_errorIndicator->size() + portDistance);
        errorRect.setHeight(Port::portSize);
    } else {
        errorRect.setWidth(0);
        errorRect.setHeight(0);
    }

    double w = nameRect.width() + 2 * portDistance + errorRect.width();

    double h = m_fontHeight + 2 * portDistance;

    QString id = " " + QString::number(m_id);
    QRect idRect = fm.boundingRect(id);

    QRect infoRectTop, infoRectBottom;
    if (m_inPorts.isEmpty()) {
        QString t = m_info;
        if (t.length() > 21) {
            t = t.left(20) + "…";
        }
        infoRectTop = fm.boundingRect(t);
    } else if (m_outPorts.size() <= 1 && !m_colormap) {
        QString t = m_info;
        if (t.length() > 21) {
            t = t.left(20) + "…";
        }
        infoRectBottom = fm.boundingRect(t);
    } else if (m_colormap) {
        //infoRectBottom = m_colormap->rect().width() + 2 * portDistance + errorRect.width();
        infoRectBottom = QRect(0, 0, m_colormap->minWidth(), m_colormap->rect().height());
    }

    {
        int idx = 0;
        for (Port *in: m_inPorts) {
            in->setPos(portDistance + idx * (portDistance + Port::portSize), 0.);
            ++idx;
        }
        w = qMax(w, 2 * portDistance + idx * (portDistance + Port::portSize) + infoRectTop.width() + idRect.width() +
                        portDistance);
    }

    {
        int idx = 0;
        for (Port *out: m_outPorts) {
            out->setPos(portDistance + idx * (portDistance + Port::portSize), h);
            ++idx;
        }
        w = qMax(w, 2 * portDistance + idx * (portDistance + Port::portSize) + infoRectBottom.width());
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

    if (m_colormap)
        m_colormap->setRect(0, 0, w - 2 * portDistance, m_colormap->height());

    setRect(0., 0., w, h);
    m_errorIndicator->setPos(errorPos());
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
    if (m_highlighted) {
        highlightPen.setColor(Qt::yellow);
        QPalette p;
        QPen pen(p.color(QPalette::Active, QPalette::Highlight), borderWidth, Qt::SolidLine, Qt::RoundCap,
                 Qt::RoundJoin);
        painter->setPen(pen);
    } else if (isSelected() && m_Status != BUSY) {
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
    painter->drawText(QPointF(portDistance, Port::portSize + m_fontHeight / 2.), m_visibleName);

    QFont font;
    QFontMetrics fm(font);

    if (m_inPorts.isEmpty()) {
        QString t = m_info;
        if (t.length() > 21) {
            t = t.left(20) + "…";
        }
        painter->drawText(QPointF(portDistance, m_fontHeight / 2.), t);
    } else if (m_outPorts.size() <= 1 && !m_colormap) {
        QString t = m_info;
        if (t.length() > 21) {
            t = t.left(20) + "…";
        }
        painter->drawText(QPointF(portDistance + (m_outPorts.size()) * (portDistance + Port::portSize),
                                  2 * Port::portSize + m_fontHeight / 2.),
                          t);
    }

    QString id = QString::number(m_id);
    QRect idRect = fm.boundingRect(id);
    painter->drawText(rect().x() + rect().width() - idRect.width() - 2. * portDistance, m_fontHeight / 2., id);
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
    m_replayOutput->setVisible(!multiSel);

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
            if (baseName.endsWith("Vtkm")) {
                baseName = baseName.left(baseName.size() - 4);
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
        DataFlowNetwork::setParameter(vistle::message::Id::Vistle, QString("position[%1]").arg(id()),
                                      vistle::ParamVector(x, y));
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

QList<Port *> Module::ports(PortKind kind) const
{
    switch (kind) {
    case Input:
        return m_inPorts;
    case Output:
        return m_outPorts;
    case Parameter:
        return QList<Port *>();
    }

    return QList<Port *>();
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

void Module::setDisplayName(QString name)
{
    m_displayName = name;
    updateText();
}

void Module::updateText()
{
    //m_visibleName = QString("%1_%2").arg(name, QString::number(m_id));
    m_visibleName = m_name;
    if (!m_displayName.isEmpty()) {
        m_visibleName = m_displayName;
    } else if (m_inPorts.isEmpty()) {
    } else if (m_outPorts.size() <= 1 && !m_colormap) {
    } else {
        if (!m_info.isEmpty()) {
            m_visibleName = m_name;
            if (m_colormap) {
                m_visibleName.clear();
            }
            if (m_name == "IndexManifolds")
                m_visibleName = "Index";
            if (m_name.startsWith("IsoSurface"))
                m_visibleName = "Iso";
            if (m_name.startsWith("CuttingSurface"))
                m_visibleName = "Cut";
            if (m_name.startsWith("AddAttribute"))
                m_visibleName = "Attr";
            if (m_name.startsWith("Variant"))
                m_visibleName = "Var";
            if (m_name.startsWith("Transform"))
                m_visibleName = "X";
            if (m_name.startsWith("Thicken"))
                m_visibleName = "Th";
            if (m_name.startsWith("VortexCriteria"))
                m_visibleName = "Vortex";
            if (!m_visibleName.isEmpty())
                m_visibleName += ":";
            m_visibleName += m_info;
            if (m_visibleName.length() > 21) {
                m_visibleName = m_visibleName.left(20) + "…";
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
        DataFlowNetwork::setParameter(vistle::message::Id::Vistle, QString("layer[%1]").arg(id()),
                                      vistle::Integer(layer));
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
    if (!port)
        return nullptr;
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

void Module::createParameterPopup()
{
    m_parameterPopup = new ParameterPopup(QStringList{});
    connect(m_parameterPopup, &ParameterPopup::parameterSelected, this, [this](const QString &param) {
        // Handle parameter button click
        vistle::Port from(m_parameterConnectionRequest.moduleId, m_parameterConnectionRequest.paramName.toStdString(),
                          vistle::Port::Type::PARAMETER);
        vistle::Port to(m_id, param.toStdString(), vistle::Port::Type::PARAMETER);
        vistle::VistleConnection::the().connect(&from, &to);
        m_parameterPopup->close();
    });
}

void Module::showParameters(const ParameterConnectionRequest &request)
{
    auto params = scene()->getModuleParameters(m_id);
    m_parameterPopup->setParameters(params);
    m_parameterPopup->setSearchText(request.paramName);
    m_parameterConnectionRequest = request;
    m_parameterPopup->move(request.pos);
    m_parameterPopup->show();
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
            auto id = mod->id();
            auto p = DataFlowNetwork::getModulePosition(id);
            if (pos().x() != p.x() || pos().y() != p.y()) {
                mod->sendPosition();
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

QPointF Module::errorPos() const
{
    if (!m_errorIndicator->isVisible())
        return QPointF();
    return QPointF(rect().right() - m_errorIndicator->size() - 0.5 * portDistance,
                   portDistance + 1.5 * Port::portSize - 0.5 * m_errorIndicator->size());
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
        m_borderColor = Qt::gray;
        if (scene()) {
            m_borderColor = scene()->highlightColor();
        }
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
    case CRASHED:
        toolTip = "Crashed";
        m_color = Qt::darkGray;
        m_borderColor = Qt::black;
        break;
    }
    if (m_errorState && m_Status != CRASHED) {
        m_borderColor = Qt::red;
    }
    m_errorIndicator->setVisible(m_errorState);

    if (!m_info.isEmpty()) {
        toolTip += " - " + m_info;
    }

    m_cancelExecAct->setEnabled(status == BUSY || status == EXECUTING);
    for (auto *a: {m_toggleOutputStreaming, m_attachDebugger, m_execAct}) {
        a->setEnabled(status != SPAWNING && status != CRASHED);
    }

    if (m_statusText.isEmpty()) {
        if (isEnabled())
            setToolTip(toolTip);
        m_tooltip = toolTip;
    }

    doLayout();
    update();
}

void Module::setToolTip(QString text)
{
    QString tt = m_name;
    if (!text.isEmpty()) {
        tt += "\n" + text;
    }
    Base::setToolTip(tt);
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

void Module::setInfo(QString text, int type)
{
    m_info = text;
    updateText();
}

void Module::setColormap(int source, QString species, const Range &range, const std::vector<vistle::RGBA> *rgba)
{
    if (species.isEmpty() || !rgba) {
        delete m_colormap;
        m_colormap = nullptr;
        return;
    }

    if (!m_colormap)
        m_colormap = new Colormap(this);
    m_colormap->setData(source, species, range, rgba);
    //m_colormap->setPos(portDistance, rect().top() + Port::portSize + portDistance);
    m_colormap->setPos(portDistance, rect().top() + 2 * Port::portSize + portDistance);

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
    m_errorIndicator->addMessage(message, type);
    m_messages.push_back({type, message});
    if (type == vistle::message::SendText::Error) {
        if (!m_errorState) {
            m_errorState = true;
            setStatus(m_Status);
            doLayout();
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
