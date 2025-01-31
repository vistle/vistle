#ifndef GUI_PORT_H
#define GUI_PORT_H

#include <QGraphicsItem>
#include <QAction>
#include <memory>

namespace vistle {
class Port;
}

namespace gui {

class Module;

class Port: public QObject, public QGraphicsRectItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    typedef QGraphicsRectItem Base;

public:
    static const double portSize;

    enum Type {
        Parameter,
        Input,
        Output,
    };

    Port(const vistle::Port *port, Module *parent);

    bool valid() const;
    Type portType() const;
    Module *module() const;
    QString name() const;
    const vistle::Port *vistlePort() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QPointF scenePos() const;

    bool operator<(const Port &other) const;
    bool operator==(const Port &other) const;
    void setInfo(QString info, int type);

signals:
    void clicked(Port *port);
    void selectConnected(int direction, int id, QString port);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    void createTooltip();
    void createMenus();
    void createGeometry();

    QMenu *m_portMenu = nullptr;
    QAction *m_selectUpstreamAct = nullptr, *m_selectDownstreamAct = nullptr, *m_selectConnectedAct = nullptr;
    QAction *m_disconnectAct = nullptr;

    Type m_portType; //< type of port
    const vistle::Port *m_port = nullptr;
    QColor m_color;
    Module *m_module = nullptr;
    int m_moduleId = 0;
    QString m_name;
    QString m_info;
    QString m_type, m_mapped, m_geometry, m_mapping;
    QString m_tooltip;
};

} //namespace gui

Q_DECLARE_METATYPE(gui::Port::Type)
#endif // VSLOT_H
