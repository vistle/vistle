#ifndef VMODULE_H
#define VMODULE_H

#include <QRect>
#include <QPolygon>
#include <QPoint>
#include <QList>
#include <QString>
#include <QSize>
#include <QGraphicsRectItem>
#include <QAction>

#include <boost/uuid/uuid.hpp>

#include <userinterface/vistleconnection.h>

#include "port.h"

namespace gui {

class Connection;
class Scene;

class Module : public QObject, public QGraphicsRectItem
{
   Q_OBJECT
   Q_INTERFACES(QGraphicsItem)

   typedef QGraphicsRectItem Base;

   static const double constexpr portDistance = 3.;

signals:
    void mouseClickEvent();

public:
    enum Status { SPAWNING, // grey
                  INITIALIZED,	// green
                  KILLED,			// red
                  BUSY,			// yellow
                  ERROR };		// black

    Module(QGraphicsItem *parent = 0, QString name = 0);
    virtual ~Module();
    QRectF boundingRect() const;                        // re-implemented
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget);                        // re-implemented
    ///\todo this functionality is unnecessary, push functionality to port
    QPointF portPos(const Port *port) const;
    void setStatus(Module::Status status);

    void addPort(vistle::Port *port);

    // vistle methods
    QString name() const;
    void setName(QString name);

    int id() const;
    void setId(int id);

    boost::uuids::uuid spawnUuid() const;
    void setSpawnUuid(const boost::uuids::uuid &uuid);

    template<class T>
    void setParameter(QString name, const T &value) const;
    template<class T>
    vistle::ParameterBase<T> *getParameter(QString name) const;
    void sendPosition() const;
    bool isPositionValid() const;
    void setPositionValid();

    Port *getGuiPort(vistle::Port *port) const;
    vistle::Port *getVistlePort(Port *port) const;

    Scene *scene() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
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

    int m_id = 0;
    boost::uuids::uuid m_spawnUuid;

    ///\todo add data structure for the module information
    QString m_name;
    Module::Status m_Status = SPAWNING;
    bool m_validPosition = false;

    QList<Port *> m_inPorts, m_outPorts, m_paramPorts;
    QColor m_color;
    qreal m_fontHeight = 0.;
    std::map<vistle::Port *, Port *> m_vistleToGui;
    std::map<Port *, vistle::Port *> m_guiToVistle;

    QColor m_borderColor;
};

template <class T>
void Module::setParameter(QString name, const T &value) const {

   vistle::VistleConnection::the().setParameter(id(), name.toStdString(), value);
}

template <class T>
vistle::ParameterBase<T> *Module::getParameter(QString name) const {

   return dynamic_cast<vistle::ParameterBase<T> *>(vistle::VistleConnection::the().getParameter(id(), name.toStdString()));
}

} //namespace gui

#endif // VMODULE_H
