#include "errorindicator.h"
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
#include <QApplication>
#include <QStatusTipEvent>
#include <QToolTip>

#include "port.h"


namespace gui {

ErrorIndicator::ErrorIndicator(Module *parent): Base(parent), m_module(parent)
{
    createGeometry();
    createTooltip();

    setAcceptHoverEvents(true);
    setCursor(Qt::ArrowCursor);
}

double ErrorIndicator::size() const
{
    return Port::portSize * 1.2;
}

QVariant ErrorIndicator::itemChange(GraphicsItemChange change, const QVariant &value)
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

Module *ErrorIndicator::module() const
{
    return m_module;
}

void ErrorIndicator::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QBrush brush(Port::outColor, Qt::SolidPattern);
    painter->setBrush(brush);

    painter->setPen(Qt::NoPen);
    painter->drawEllipse(0., 0., size(), size());

    painter->setPen(Qt::black);

    QString text = "!";

    QFont font;
    font.setBold(true);
    //font.setPointSize(font.pointSize() - 2);
    QFontMetrics fm(font);
    painter->setFont(font);
    QRect textR = fm.boundingRect(text);
    float left = (size() - textR.width() * 2.) / 2.;
    painter->drawText(QPointF(left, (size() / 2. + textR.height()) / 2.), text);
}

QPointF ErrorIndicator::scenePos() const
{
    return mapToScene(mapFromItem(m_module, m_module->errorPos()));
}

void ErrorIndicator::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Base::mousePressEvent(event);
}

void ErrorIndicator::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (rect().contains(event->pos())) {
        emit clicked(this);
    }

    Base::mouseReleaseEvent(event);
}

void ErrorIndicator::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QStatusTipEvent statusTipEvent(m_tooltip);
    QCoreApplication::sendEvent(DataFlowView::the(), &statusTipEvent);
}

void ErrorIndicator::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QStatusTipEvent statusTipEvent("");
    QCoreApplication::sendEvent(DataFlowView::the(), &statusTipEvent);
}


void ErrorIndicator::addMessage(QString text, int type)
{
    if (type != vistle::message::SendText::Error) {
        return;
    }
    m_messages.push_back(text);
    createTooltip();
}

void ErrorIndicator::createTooltip()
{
    m_tooltip.clear();
    if (!m_messages.empty()) {
        if (m_messages.size() > 1) {
            m_tooltip += "[" + QString::number(m_messages.size()) + " messages...]\n";
        }
        m_tooltip += m_messages.back();
    }

    if (isEnabled())
        setToolTip(m_tooltip);
    update();
}

void ErrorIndicator::createGeometry()
{
    setRect(0., 0., size(), size());
    update();
}

} //namespace gui
