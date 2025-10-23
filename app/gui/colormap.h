#ifndef VISTLE_GUI_COLORMAP_H
#define VISTLE_GUI_COLORMAP_H

#include <QGraphicsItem>
#include <QAction>
#include <memory>
#include <vistle/core/message/colormap.h>
#include <vistle/core/scalar.h>

#if 0
typedef std::array<vistle::Float, 2> Range;
Q_DECLARE_METATYPE(Range)
#endif

namespace gui {
class Module;
typedef std::array<vistle::Float, 2> Range;
} // namespace gui
Q_DECLARE_METATYPE(gui::Range)

namespace gui {


class Colormap: public QObject, public QGraphicsRectItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    typedef QGraphicsRectItem Base;

public:
    Colormap(Module *parent);
    void setData(int source, QString species, const Range &range, const std::vector<vistle::RGBA> *rgba);

    Module *module() const;
    bool valid() const;
    double min() const;
    double max() const;
    QString name() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QPointF scenePos() const;
    float height() const;
    float minWidth() const;

    void clear();

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    void createTooltip();
    void createMenus();
    void createGeometry();

    Module *m_module = nullptr;
    QString m_tooltip;
    QString m_name;
    double m_min = 0.;
    double m_max = 1.;
    std::vector<vistle::RGBA> m_rgba;
};

} //namespace gui

#endif
