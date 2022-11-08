#ifndef VSCENE_H
#define VSCENE_H

#include <QList>
#include <QString>
#include <QGraphicsScene>

#include "port.h"
#include <vistle/core/uuid.h>

#include <cassert>
#include <set>

namespace vistle {
class VistleConnection;
class StateTracker;
} // namespace vistle

namespace gui {

class Module;
class Connection;
class MainWindow;
class ModuleBrowser;

enum SelectionDirection {
    SelectConnected,
    SelectUp,
    SelectUpstream,
    SelectDown,
    SelectDownstream,
};

class DataFlowNetwork: public QGraphicsScene {
    Q_OBJECT

public:
    DataFlowNetwork(vistle::VistleConnection *conn, MainWindow *mw, QObject *parent = 0);
    ~DataFlowNetwork();

    enum Layers {
        AllLayers = -1,
    };

    ModuleBrowser *moduleBrowser() const;

    void addModule(int hub, QString modName, Qt::Key direction);
    void addModule(int hub, QString modName, QPointF dropPos);

    void addConnection(Port *portFrom, Port *portTo, bool sendToController = false);
    void removeConnection(Port *portFrom, Port *portTo, bool sendToController = false);
    void removeConnections(Port *port, bool sendToController = false);
    void setConnectionHighlights(Port *port, bool highlight);

    Module *newModule(QString modName);
    Module *findModule(int id) const;
    Module *findModule(const boost::uuids::uuid &spawnUuid) const;

    bool moveModule(int moduleId, float x, float y);

    QColor highlightColor() const;
    QRectF computeBoundingRect(int layer = AllLayers) const;
    bool isDark() const;
public slots:
    void addModule(int moduleId, const boost::uuids::uuid &spawnUuid, QString name);
    void deleteModule(int moduleId);
    void moduleStateChanged(int moduleId, int stateBits);
    void newPort(int moduleId, QString portName);
    void deletePort(int moduleId, QString portName);
    void newConnection(int fromId, QString fromName, int toId, QString toName);
    void deleteConnection(int fromId, QString fromName, int toId, QString toName);
    void moduleStatus(int id, QString status, int prio);
    void itemInfoChanged(QString text, int type, int id, QString port);
    void moduleMessage(int senderId, int type, QString message);
    void clearMessages(int moduleId);
    void messagesVisibilityChanged(int moduleId, bool visible);

    void emphasizeConnections(QList<Module *> modules);
    void visibleLayerChanged(int layer);
    void updateConnectionVisibility();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override; //< re-implemented
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override; //< re-implemented
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override; //< re-implemented
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;

private:
    QList<Module *> m_moduleList; //< list of modules
    QGraphicsLineItem *m_Line; //< the intermediate line drawn between modules
    QColor m_LineColor; //< color of the line
    QPointF vLastPoint; //< intermediate previous point for connection drawing
    bool m_mousePressed; //< boolean for keeping track of if a click is made
    Port *startPort; //< starting port for module connection
    Module *startModule; //< starting module for making connection
    Module *endModule; //< ending module for making connection
    QPointF lastDropPos;
    ///\todo push this functionality to vHandler
    vistle::VistleConnection *m_vistleConnection;
    vistle::StateTracker &m_state;
    MainWindow *m_mainWindow = nullptr;

    struct ConnectionKey {
        ConnectionKey(Port *p1, Port *p2): port1(p1), port2(p2)
        {
            if (*port2 < *port1)
                std::swap(port1, port2);
        }

        bool valid() const { return port1->valid() && port2->valid(); }

        Port *port1, *port2;

        bool operator<(const ConnectionKey &c) const
        {
            assert(valid());
            assert(c.valid());

            if (*port1 == *c.port1)
                return *port2 < *c.port2;

            return *port1 < *c.port1;
        }
    };

    typedef std::map<ConnectionKey, Connection *> ConnectionMap;
    ConnectionMap m_connections;

    QColor m_highlightColor;

    void selectModules(const std::set<int> &moduleIds);
private slots:
    void createModuleCompound();
    void selectConnected(int direction, int id, QString port = QString());
};

} //namespace gui

#endif // VSCENE_H
