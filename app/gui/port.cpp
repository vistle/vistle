/*********************************************************************************/
/** \file port.cpp
 *
 * graphical representation for a module port
 */
/**********************************************************************************/
#include "port.h"
#include "module.h"
#include "dataflownetwork.h"
#include "dataflowview.h"
#include <vistle/core/port.h>
#include <cassert>

#include <QCursor>
#include <QAction>
#include <QMenu>
#include <QRadialGradient>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QStatusTipEvent>
#include <QCoreApplication>

#include <vistle/config/file.h>
#include <vistle/config/value.h>

namespace gui {

static QColor InColor(200, 30, 30);
static QColor ParamColor(30, 30, 200);
static QColor DisabledColor(150, 150, 150);
static QColor OptionalColor(200, 200, 30);
//static QColor OutColor = OptionalColor;
static QColor OutColor = InColor;

double Port::portSize = 14.;
QColor Port::outColor = OutColor;

void Port::configure()
{
    vistle::config::File config("gui");
    portSize = *config.value<double>("module", "port_size", 14.);
}

Port::Port(const vistle::Port *port, Module *parent)
: Base(parent)
, m_port(port)
, m_module(parent)
, m_moduleId(port->getModuleID())
, m_name(QString::fromStdString(port->getName()))
{
    switch (port->getType()) {
    case vistle::Port::INPUT:
        m_portType = Input;
        break;
    case vistle::Port::OUTPUT:
        m_portType = Output;
        break;
    case vistle::Port::PARAMETER:
        m_portType = Parameter;
        break;
    default:
        assert("port type not handled" == 0);
        break;
    }

    createGeometry();
    createMenus();
    createTooltip();
}

void Port::createMenus()
{
    connect(this, &Port::selectConnected, m_module, &Module::selectConnected);

    m_portMenu = new QMenu();

    if (m_portType == Input) {
        m_selectUpstreamAct = new QAction("Select Upstream", this);
        m_selectUpstreamAct->setStatusTip("Select all modules feeding data to this port");
        connect(m_selectUpstreamAct, &QAction::triggered,
                [this]() { emit selectConnected(SelectUpstream, m_moduleId, m_name); });
        m_portMenu->addAction(m_selectUpstreamAct);
    }

    m_selectConnectedAct = new QAction("Select Connected", this);
    m_selectConnectedAct->setStatusTip("Select all modules connected to this port");
    connect(m_selectConnectedAct, &QAction::triggered,
            [this]() { emit selectConnected(SelectConnected, m_moduleId, m_name); });
    m_portMenu->addAction(m_selectConnectedAct);

    if (m_portType == Output) {
        m_selectDownstreamAct = new QAction("Select Downstream", this);
        m_selectDownstreamAct->setStatusTip("Select all modules this port feeds into");
        connect(m_selectDownstreamAct, &QAction::triggered,
                [this]() { emit selectConnected(SelectDownstream, m_moduleId, m_name); });
        m_portMenu->addAction(m_selectDownstreamAct);
    }

    m_portMenu->addSeparator();

    m_disconnectAct = new QAction("Remove Connections", this);
    m_disconnectAct->setStatusTip("Remove connections to all other modules");
    connect(m_disconnectAct, &QAction::triggered, [this]() {
        auto *sc = dynamic_cast<DataFlowNetwork *>(scene());
        assert(sc);
        sc->removeConnections(this, true);
    });
    m_portMenu->addAction(m_disconnectAct);
}

QVariant Port::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemEnabledHasChanged) {
        if (isEnabled()) {
            setToolTip(m_tooltip);
        } else {
            setToolTip("");
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void Port::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    bool hasCon = !m_port->connections().empty();
    m_disconnectAct->setEnabled(hasCon);
    m_selectConnectedAct->setEnabled(hasCon);
    if (m_selectDownstreamAct)
        m_selectDownstreamAct->setEnabled(hasCon);
    if (m_selectUpstreamAct)
        m_selectUpstreamAct->setEnabled(hasCon);
    m_portMenu->popup(event->screenPos());
}

QString Port::name() const
{
    return m_name;
}

bool Port::valid() const
{
    return m_port != nullptr;
}

const vistle::Port *Port::vistlePort() const
{
    DataFlowNetwork *sc = dynamic_cast<DataFlowNetwork *>(scene());
    auto p = sc->state().portTracker()->getPort(m_moduleId, m_name.toStdString());
    return p;
}

bool Port::operator<(const Port &other) const
{
    if (m_moduleId == other.m_moduleId) {
        return m_name < other.m_name;
    }
    return m_moduleId < other.m_moduleId;
}

bool Port::operator==(const Port &other) const
{
    return m_moduleId == other.m_moduleId && m_name == other.m_name;
}

Port::Type Port::portType() const
{
    return m_portType;
}

Module *Port::module() const
{
    return m_module;
}

void Port::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // ball
    QRadialGradient gradient(4, 4, portSize / 2);
    gradient.setCenter(6, 6);
    gradient.setFocalPoint(6, 6);

    //painter->setPen(QPen(Qt::black, 0));
    painter->setPen(Qt::NoPen);
    gradient.setColorAt(0, Qt::white);
    gradient.setColorAt(0, m_color);
    gradient.setColorAt(1, m_color);
    painter->setBrush(gradient);
    painter->drawRect(0, 0, portSize, portSize);

    if (m_portType == Port::Input) {
        if (vistlePort() && vistlePort()->isConnected() && !(vistlePort()->flags() & vistle::Port::COMBINE_BIT)) {
            painter->setPen(QPen(Qt::black, 2));
            painter->setBrush(Qt::NoBrush);
            auto s = .25 * portSize, b = .75 * portSize;
            painter->drawLine(QPointF(s, s), QPointF(b, b));
            painter->drawLine(QPointF(s, b), QPointF(b, s));
        }
    }

    if (m_portType == Port::Output) {
        QString text = m_mapped;
        if (text.isEmpty()) {
            text = m_type;
        }

        auto shortenType = [](QString text) {
            if (text == "Empty") {
                text = "";
            }
            if (text == "Points") {
                text = ".";
            }
            if (text == "Lines") {
                text = "-";
            }
            if (text == "Triangles") {
                text = "△";
            }
            if (text == "Quads") {
                text = "□";
            }
            if (text == "Polygons") {
                text = "p";
            }
            if (text == "UniformGrid") {
                text = "c";
            }
            if (text == "LayerGrid") {
                text = "h";
            }
            if (text == "RectilinearGrid") {
                text = "r";
            }
            if (text == "StructuredGrid") {
                text = "s";
            }
            if (text == "UnstructuredGrid") {
                text = "u";
            }
            if (text.startsWith("Texture")) {
                text = "t";
            }
            if (text.startsWith("Scalar")) {
                text = text.right(1);
            }
            if (text.startsWith("Index")) {
                text = "x";
            }
            return text;
        };

        painter->setPen(Qt::black);

        QString type = shortenType(text);

        QFont font;
        font.setBold(true);
        font.setPointSize(font.pointSize() - 2);
        QFontMetrics fm(font);
        QRect typeR = fm.boundingRect(type);
        float left = (Port::portSize - typeR.width()) / 2.;

        QString geo = shortenType(m_geometry);
        QRect geoR;
        //float left = (Port::portSize + typeR.width()*3./4. - geoR.width()*2.) / 2.;
        if (!geo.isEmpty()) {
            QFont small = font;
            small.setPointSize(font.pointSize() - 2);
            QFontMetrics fm(small);
            geoR = fm.boundingRect(geo);
            left -= geoR.width() * 3. / 4.;

            painter->setFont(small);
            painter->drawText(QPointF(left + typeR.width() + geoR.width() / 4., (Port::portSize + typeR.height()) / 2.),
                              geo);

            painter->setFont(font);
            QString mapping = m_mapping == "Vertex" ? "." : "";
            if (!mapping.isEmpty()) {
                painter->drawText(QPointF(left + typeR.width(), (Port::portSize - typeR.height() / 2.) / 2.), mapping);
            }
        }

        painter->drawText(QPointF(left, (Port::portSize * 3. / 4. + typeR.height()) / 2.), type);
    }
}

QPointF Port::scenePos() const
{
    return mapToScene(mapFromItem(m_module, m_module->portPos(this)));
}

void Port::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    auto *sc = dynamic_cast<DataFlowNetwork *>(scene());
    assert(sc);
    sc->setConnectionHighlights(this, true);

    QStatusTipEvent statusTipEvent(m_statustip);
    QCoreApplication::sendEvent(DataFlowView::the(), &statusTipEvent);
}

void Port::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    auto *sc = dynamic_cast<DataFlowNetwork *>(scene());
    assert(sc);
    sc->setConnectionHighlights(this, false);

    QStatusTipEvent statusTipEvent("");
    QCoreApplication::sendEvent(DataFlowView::the(), &statusTipEvent);
}

void Port::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Base::mousePressEvent(event);
}

void Port::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
#if 0
   if (rect().contains(event->pos())) {
      emit clicked(this);
      return;
   }
#endif

    DataFlowNetwork *sc = dynamic_cast<DataFlowNetwork *>(scene());
    QGraphicsItem *item = scene()->itemAt(event->scenePos(), QTransform());
    if (Port *dest = dynamic_cast<Port *>(item)) {
        if (portType() == Output && dest->portType() == Input) {
            sc->addConnection(this, dest, true);
        } else if (portType() == Input && dest->portType() == Output) {
            sc->addConnection(dest, this, true);
        }
    }

    Base::mouseReleaseEvent(event);
}

void Port::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Base::mouseMoveEvent(event);
}

void Port::setInfo(int flag, int type)
{
    using namespace vistle::message;

    assert(type == ItemInfo::PortEnableState);
    switch (type) {
    case ItemInfo::PortEnableState: {
        switch (flag) {
        case ItemInfo::Enabled:
            m_enableState = Enabled;
            m_color = m_portType == Input ? InColor : (m_portType == Output ? OutColor : ParamColor);
            break;
        case ItemInfo::Disabled:
            assert(m_portType == Output);
            m_color = DisabledColor;
            break;
        case ItemInfo::Optional:
            assert(m_portType == Input);
            m_color = OptionalColor;
            break;
        }
        break;
    }
    }
    update();
}

void Port::setInfo(QString text, int type)
{
    using vistle::message::ItemInfo;
    switch (type) {
    case ItemInfo::Port:
        m_info = text;
        createTooltip();
        break;
    case ItemInfo::PortType:
        m_type = text;
        break;
    case ItemInfo::PortMapped:
        m_mapped = text;
        break;
    case ItemInfo::PortGeometry:
        m_geometry = text;
        break;
    case ItemInfo::PortMapping:
        m_mapping = text;
        break;
    }
}

void Port::createTooltip()
{
    m_tooltip.clear();
    if (m_port) {
        QString desc = QString::fromStdString(m_port->getDescription());
        QString name = QString::fromStdString(m_port->getName());
        m_tooltip = desc;
        if (!m_tooltip.isEmpty()) {
            m_tooltip += " ";
        }
        m_tooltip += "(" + name + ")";
    }
    m_statustip = m_tooltip;

    if (!m_info.isEmpty()) {
        m_tooltip += "\n";
        m_tooltip += m_info;
        m_statustip += " - " + m_info;
    }

    if (isEnabled())
        setToolTip(m_tooltip);
}

void Port::createGeometry()
{
    setCursor(Qt::CrossCursor);
    setAcceptHoverEvents(true);

    switch (m_portType) {
    case Port::Input:
        m_color = InColor;
        break;
    case Port::Output:
        m_color = OutColor;
        break;
    case Port::Parameter:
        m_color = ParamColor;
        break;
    }

    setRect(0., 0., portSize, portSize);
    update();
}

} //namespace gui
