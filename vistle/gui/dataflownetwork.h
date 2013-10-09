#ifndef VSCENE_H
#define VSCENE_H

#include <QList>
#include <QString>
#include <QGraphicsScene>

#include "port.h"
#include <boost/uuid/uuid.hpp>

namespace vistle {
class VistleConnection;
}

namespace gui {

class Module;
class Connection;

class DataFlowNetwork: public QGraphicsScene
{
   Q_OBJECT

public:
    DataFlowNetwork(vistle::VistleConnection *conn, QObject *parent = 0);
    ~DataFlowNetwork();

    void addModule(QString modName, QPointF dropPos);

    void addConnection(Port *portFrom, Port *portTo, bool sendToController=false);
    void removeConnection(Port *portFrom, Port *portTo, bool sendToController=false);

    Module *findModule(int id) const;
    Module *findModule(const boost::uuids::uuid &spawnUuid) const;

    QColor highlightColor() const;

public slots:
    void addModule(int moduleId, const boost::uuids::uuid &spawnUuid, QString name);
    void deleteModule(int moduleId);
    void moduleStateChanged(int moduleId, int stateBits);
    void newPort(int moduleId, QString portName);
    void newConnection(int fromId, QString fromName, int toId, QString toName);
    void deleteConnection(int fromId, QString fromName, int toId, QString toName);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);      //< re-implemented
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);       //< re-implemented
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);    //< re-implemented

private:
    QList<Module *> m_moduleList;                                 //< list of modules
    QGraphicsLineItem *m_Line;                        //< the intermediate line drawn between modules
    QColor m_LineColor;                                         //< color of the line
    QPointF vLastPoint;                                         //< intermediate previous point for connection drawing
    bool m_mousePressed;                                           //< boolean for keeping track of if a click is made
    Port *startPort;                                  //< starting port for module connection
    Module *startModule;                              //< starting module for making connection
    Module *endModule;                                //< ending module for making connection

    ///\todo push this functionality to vHandler
    vistle::VistleConnection *m_vistleConnection;

    struct ConnectionKey {
       ConnectionKey(Port *p1, Port *p2)
       : port1(p1), port2(p2)
       {
          if (port1 > port2)
             std::swap(port1, port2);
       }

       Port *port1, *port2;

       bool operator<(const ConnectionKey &c) const {

          if (port1 < c.port1) {
             return true;
          }
          if (c.port1 < port1) {
             return false;
          }

          return (port2 < c.port2);
       }
    };

    typedef std::map<ConnectionKey, Connection *> ConnectionMap;
    ConnectionMap m_connections;

    QColor m_highlightColor;
};

} //namespace gui

#endif // VSCENE_H
