#ifndef VSCENE_H
#define VSCENE_H

#include <QList>
#include <QString>
#include <QGraphicsItem>
#include <QGraphicsScene>

#include <userinterface/vistleconnection.h>

#include "module.h"
#include "connection.h"
#include "port.h"

namespace gui {

class MainWindow;

class Scene : public QGraphicsScene
{
public:
    Scene(QObject *parent = 0);
    ~Scene();
    void addModule(QString modName, QPointF dropPos);
    void setVistleConnection(vistle::VistleConnection *runnner);
    void addModule(int moduleId, const boost::uuids::uuid &spawnUuid, QString name);
    void deleteModule(int moduleId);
    void addConnection(Port *portFrom, Port *portTo, bool sendToController=false);
    void removeConnection(Port *portFrom, Port *portTo, bool sendToController=false);

    void setMainWindow(MainWindow *w);
    MainWindow *mainWindow() const;

    Module *findModule(int id) const;
    Module *findModule(const boost::uuids::uuid &spawnUuid) const;

    QColor highlightColor() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);      //< re-implemented
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);       //< re-implemented
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);    //< re-implemented

private:
    QList<Module *> m_moduleList;                                 //< list of modules
    QGraphicsLineItem *m_Line = nullptr;                        //< the intermediate line drawn between modules
    QColor m_LineColor;                                         //< color of the line
    QPointF vLastPoint;                                         //< intermediate previous point for connection drawing
    bool vMouseClick;                                           //< boolean for keeping track of if a click is made
    Port *startPort = nullptr;                                  //< starting port for module connection
    Module *startModule = nullptr;                              //< starting module for making connection
    Module *endModule = nullptr;                                //< ending module for making connection

    ///\todo push this functionality to vHandler
    vistle::VistleConnection *m_vistleConnection = nullptr;

    MainWindow *m_mainWindow = nullptr;

    struct ConnectionKey {
       ConnectionKey(Port *p1, Port *p2)
       : port1(p1), port2(p2)
       {
          if (port1 > port2)
             std::swap(port1, port2);
       }

       Port *port1=nullptr, *port2=nullptr;

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
