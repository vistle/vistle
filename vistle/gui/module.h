#ifndef GUI_MODULE_H
#define GUI_MODULE_H

#include <QRect>
#include <QPolygon>
#include <QPoint>
#include <QList>
#include <QString>
#include <QSize>
#include <QGraphicsRectItem>
#include <QAction>

#include <core/uuid.h>

#include <userinterface/vistleconnection.h>

#include "port.h"

namespace gui {

class Connection;
class DataFlowNetwork;

class Module : public QObject, public QGraphicsRectItem
{
   Q_OBJECT
   Q_INTERFACES(QGraphicsItem)

   typedef QGraphicsRectItem Base;

   static const double portDistance;

signals:
    void mouseClickEvent();

public:
    enum Status { SPAWNING,
                  INITIALIZED,
                  KILLED,
                  BUSY,
                  ERROR_STATUS };

    Module(QGraphicsItem *parent = nullptr, QString name = QString());
    virtual ~Module();
    QRectF boundingRect() const;                        // re-implemented
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget);                        // re-implemented
    ///\todo this functionality is unnecessary, push functionality to port
    QPointF portPos(const Port *port) const;
    void setStatus(Module::Status status);

    void addPort(const vistle::Port &port);
    void removePort(const vistle::Port &port);

    // vistle methods
    QString name() const;
    void setName(QString name);

    int id() const;
    void setId(int id);
    int hub() const;
    void setHub(int hub);

    boost::uuids::uuid spawnUuid() const;
    void setSpawnUuid(const boost::uuids::uuid &uuid);

    template<class T>
    void setParameter(QString name, const T &value) const;
    template<class T>
    std::shared_ptr<vistle::ParameterBase<T>> getParameter(QString name) const;
    void sendPosition() const;
    bool isPositionValid() const;
    void setPositionValid();

    Port *getGuiPort(const vistle::Port *port) const;
    const vistle::Port *getVistlePort(Port *port) const;

    DataFlowNetwork *scene() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    void updatePosition(QPointF newPos) const;

public slots:
    void execModule();
    void deleteModule();

private:
    void createGeometry();
    void createActions();
    void createMenus();
    void doLayout();

    QMenu *m_moduleMenu;
    QAction *m_deleteAct;
    QAction *m_execAct;

    int m_hub;
    int m_id;
    boost::uuids::uuid m_spawnUuid;

    ///\todo add data structure for the module information
    QString m_name;
    Module::Status m_Status;
    bool m_validPosition;

    QList<Port *> m_inPorts, m_outPorts, m_paramPorts;
    QColor m_color;
    qreal m_fontHeight;
    std::map<vistle::Port, Port *> m_vistleToGui;

    QColor m_borderColor;
};

template <class T>
void Module::setParameter(QString name, const T &value) const {

   vistle::VistleConnection::the().setParameter(id(), name.toStdString(), value);
}

template <class T>
std::shared_ptr<vistle::ParameterBase<T>> Module::getParameter(QString name) const {

   return std::dynamic_pointer_cast<vistle::ParameterBase<T>>(vistle::VistleConnection::the().getParameter(id(), name.toStdString()));
}

} //namespace gui

#endif // VMODULE_H
