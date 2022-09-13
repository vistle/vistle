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

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QPointF scenePos() const;

    bool operator<(const Port &other) const;
    bool operator==(const Port &other) const;

signals:
    void clicked(Port *port);
    void selectConnected(int direction, int id, QString port);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

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
};

} //namespace gui

Q_DECLARE_METATYPE(gui::Port::Type)
#endif // VSLOT_H
