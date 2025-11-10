#ifndef VISTLE_GUI_ERRORINDICATOR_H
#define VISTLE_GUI_ERRORINDICATOR_H

#include <QGraphicsItem>
#include <QAction>
#include <memory>

namespace gui {

class Module;

class ErrorIndicator: public QObject, public QGraphicsEllipseItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    typedef QGraphicsEllipseItem Base;

public:
    ErrorIndicator(Module *parent);

    Module *module() const;
    QString message() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QPointF scenePos() const;

    void addMessage(QString info, int type);
    void clear();
    double size() const;

signals:
    void clicked();

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
    std::vector<QString> m_messages;
    QString m_tooltip;
};

} //namespace gui

#endif
