#include "colormap.h"
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

Colormap::Colormap(Module *parent): Base(parent), m_module(parent)
{
    createGeometry();
    createTooltip();

    setAcceptHoverEvents(true);
}

QVariant Colormap::itemChange(GraphicsItemChange change, const QVariant &value)
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

Module *Colormap::module() const
{
    return m_module;
}

bool Colormap::valid() const
{
    return !m_name.isEmpty() && !m_rgba.empty();
}

void Colormap::setData(int source, QString species, const Range &range, const std::vector<vistle::RGBA> *rgba)
{
    m_name = species;
    m_min = range[0];
    m_max = range[1];

    if (rgba) {
        m_rgba = *rgba;
    } else {
        m_rgba.clear();
    }

    createTooltip();
    update();
}

void Colormap::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    float width = rect().width();

    if (!m_rgba.empty()) {
        QBrush brush(Port::outColor, Qt::SolidPattern);
        painter->setBrush(brush);
        painter->setPen(Qt::NoPen);

        for (size_t i = 0; i < m_rgba.size(); ++i) {
            const vistle::RGBA &c = m_rgba[i];
            QColor color(c[0], c[1], c[2], c[3]);
            painter->setBrush(QBrush(color, Qt::SolidPattern));
            float x = (width * i) / m_rgba.size();
            float w = (width * (i + 1)) / m_rgba.size() - x;
            QRectF rect{x, 0., w, height()};
            painter->drawRect(rect);
        }
    }

    painter->setPen(Qt::black);
    QFont font;
#if 0
    font.setBold(true);
    font.setPointSize(font.pointSize() - 2);
#endif
    QFontMetrics fm(font);
    painter->setFont(font);
    float top = (height() + fm.height()) / 2.;
    top = height() - fm.descent();
    float border = 2.;

    QString tmin, tmax;
    float left = border;
    for (int p = 4; p > 0; --p) {
        float w = border;
        {
            tmin = QString::number(m_min, 'g', p);
            QRect textR = fm.boundingRect(tmin);
            w += textR.width();
        }
        {
            tmax = QString::number(m_max, 'g', p);
            QRect textR = fm.boundingRect(tmax);
            w += textR.width();
            left = width - textR.width() - border;
        }
        w += border;
        if (w + 5 * border < width) {
            painter->drawText(QPointF(border, top), tmin);
            painter->drawText(QPointF(left, top), tmax);
            break;
        }
    }
}

QPointF Colormap::scenePos() const
{
    return mapToScene(mapFromItem(m_module, m_module->errorPos()));
}

void Colormap::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
}

void Colormap::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
}

void Colormap::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QStatusTipEvent statusTipEvent(m_tooltip);
    QCoreApplication::sendEvent(DataFlowView::the(), &statusTipEvent);
}

void Colormap::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QStatusTipEvent statusTipEvent("");
    QCoreApplication::sendEvent(DataFlowView::the(), &statusTipEvent);
}

void Colormap::createTooltip()
{
    m_tooltip.clear();

    m_tooltip = QString("%1 - %2").arg(m_min).arg(m_max);

    if (isEnabled())
        setToolTip(m_tooltip);
    update();
}

float Colormap::minWidth() const
{
    return 75.f;
}

float Colormap::height() const
{
    return Port::portSize;
}

void Colormap::createGeometry()
{
    setRect(0., 0., minWidth(), height());
    update();
}

} //namespace gui
