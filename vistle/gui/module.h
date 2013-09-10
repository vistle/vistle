#ifndef VMODULE_H
#define VMODULE_H

#include <QRect>
#include <QPolygon>
#include <QPoint>
#include <QList>
#include <QString>
#include <QSize>
#include <QGraphicsItem>
#include <QAction>

#include <boost/uuid/uuid.hpp>

#include <userinterface/vistleconnection.h>

#include "port.h"

namespace gui {

class Connection;

class Module : public QObject, public QGraphicsItem
{
   Q_OBJECT
   Q_INTERFACES(QGraphicsItem)

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
    QPolygonF polygon() const
        { return baseShape; }
    void addConnection(Connection *connection);
    void removeConnection(Connection *connection);
    void clearConnections();
    ///\todo this functionality is unnecessary, push functionality to port
    QPointF portPos(Port *port);
    void setStatus(Module::Status status);

    void addParent(Module *parentMod) { parentModules.append(parentMod); }
    void addChild(Module *childMod) { childModules.append(childMod); }
    void connectParameter(Module *paramMod) { paramModules.append(paramMod); }
    bool removeParent(Module *parentMod);
    bool removeChild (Module *childMod);
    ///\todo change name to disconnectParam
    bool disconnectParameter(Module *paramMod);
    ///\todo put in map
    void setVisited() { modIsVisited = true; }
    void unsetVisited() { modIsVisited = false; }
    bool isVisited() { return modIsVisited; }

    QList<Module *> getChildModules() { return childModules; }
    QList<Module *> getParentModules() { return parentModules; }
    QList<Module *> getParamModules() { return paramModules; }
    void setKey(QString key) { m_Key = key; }
    QString getKey() { return m_Key; }
    void clearKey() { m_Key = QString(""); }

    // vistle methods
    QString name() const;
    void setName(QString name);

    int id() const;
    void setId(int id);

    boost::uuids::uuid spawnUuid() const;
    void setSpawnUuid(const boost::uuids::uuid &uuid);

    template<class T>
    void setParameter(QString name, const T &value) const;
    void sendPosition() const;

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    void updatePosition(QPointF newPos) const;

public slots:
    void copy();
    void execModule();
    void deleteModule();
    void deleteConnections();

private:
    void createPorts();
    void updatePorts();
    void createActions();
    void createMenus();
    QMenu *m_moduleMenu;
    QAction *m_copyAct;
    QAction *m_deleteAct;
    QAction *m_execAct;
    QAction *m_deleteConnectionAct;

    qreal w;                                            //< width of the module
    qreal h;                                            //< height of the module
    int xAddr;                                          //< important calculated value for the x magnitude
    int yAddr;                                          //< important calculated value for the y magnitude
    bool vMouseClick;                                   //< boolean for keeping track of whether a click was made
    QSizeF size;                                        //< size of the module
    QList<Port *> portList;                             //< list of all the ports in the module
    QPointF vLastPoint;                                 //< point for keeping track of whether a click was made
    QPolygonF baseShape;                                //< base polygon of the module
    QGraphicsPolygonItem *shape = nullptr;              //< graphics item for base polygon
    QGraphicsPolygonItem *statusShape = nullptr;        //< pointer to the shape indicating module status
    QList<Connection *> connectionList;                 //< list of connections connected to the module

    QList<Module *> parentModules;
    QList<Module *> childModules;
    QList<Module *> paramModules;
    bool modIsVisited;
    ///\todo remove the use of m_Key in the module, along with any visited bools --
    /// these can be implemented in the map itself, and leave the module out of it.
    QString m_Key;

    int m_id = 0;
    boost::uuids::uuid m_spawnUuid;

    ///\todo add data structure for the module information
    QString m_name;
    Module::Status m_Status = SPAWNING;

};

template <class T>
void Module::setParameter(QString name, const T &value) const
{
   vistle::VistleConnection::the().setParameter(id(), name.toStdString(), value);
}

} //namespace gui

#endif // VMODULE_H
