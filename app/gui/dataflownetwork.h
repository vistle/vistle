#ifndef VISTLE_GUI_DATAFLOWNETWORK_H
#define VISTLE_GUI_DATAFLOWNETWORK_H

#include <QList>
#include <QString>
#include <QGraphicsScene>

#include "port.h"
#include "colormap.h"
#include <vistle/core/uuid.h>
#include <vistle/userinterface/vistleconnection.h>

#include <cassert>
#include <set>

namespace vistle {
class StateTracker;
} // namespace vistle

namespace gui {

class Connection;
class MainWindow;
class ModuleBrowser;
class Module;

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

    const QList<Module *> &modules() const;

    Module *newModule(QString modName);
    Module *findModule(int id) const;
    Module *findModule(const boost::uuids::uuid &spawnUuid) const;
    Module *findModule(const QPointF &pos) const;
    QList<QString> getModuleParameters(int id) const;

    bool moveModule(int moduleId, float x, float y);

    QColor highlightColor() const;
    QRectF computeBoundingRect(int layer = AllLayers) const;
    bool isDark() const;
    vistle::StateTracker &state() const;

    template<class T>
    static void setParameter(int id, QString name, const T &value);

    template<class T>
    static std::shared_ptr<vistle::ParameterBase<T>> getParameter(int id, QString name);

    static QPointF getModulePosition(int id);
    static int getModuleLayer(int id);

signals:
    void toggleOutputStreaming(int moduleId, bool enable);
    void highlightModule(int moduleId);

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
    void colormapChanged(int id, int source, QString species, Range range, const std::vector<vistle::RGBA> *rgba);
    void portStateChanged(int state, int id, QString port);
    void setDisplayName(int id, QString name);
    void moduleMessage(int senderId, int type, QString message);
    void clearMessages(int moduleId);
    void messagesVisibilityChanged(int moduleId, bool visible);
    void outputStreamingChanged(int moduleId, bool enable);

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

template<class T>
void DataFlowNetwork::setParameter(int id, QString name, const T &value)
{
    vistle::VistleConnection::the().setParameter(id, name.toStdString(), value);
}

template<class T>
std::shared_ptr<vistle::ParameterBase<T>> DataFlowNetwork::getParameter(int id, QString name)
{
    return std::dynamic_pointer_cast<vistle::ParameterBase<T>>(
        vistle::VistleConnection::the().getParameter(id, name.toStdString()));
}

} //namespace gui

// required for compiling generated moc for emphasizeConnections with GCC and MSVC,
// but cannot be included at top as this prevents template instantiation for Clang in module.h
#include "module.h"
#endif
