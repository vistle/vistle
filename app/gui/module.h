#ifndef GUI_MODULE_H
#define GUI_MODULE_H

#include <QAction>
#include <QFileDialog>
#include <QGraphicsRectItem>
#include <QList>
#include <QPoint>
#include <QPolygon>
#include <QRect>
#include <QSize>
#include <QString>

#include <vistle/core/uuid.h>
#include <vistle/core/parameter.h>
#include <vistle/core/messages.h>

#include "port.h"
#include "dataflownetwork.h"

namespace gui {

class Connection;
class DataFlowNetwork;

const bool LayersAsOpacity = true;

class Module: public QObject, public QGraphicsRectItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

    typedef QGraphicsRectItem Base;

    static const double portDistance;
    static const double borderWidth;
    static bool s_snapToGrid;

public:
    enum Status { SPAWNING, INITIALIZED, KILLED, BUSY, EXECUTING, ERROR_STATUS, CRASHED };

    struct Message {
        int type;
        QString text;
    };

    Module(QGraphicsItem *parent = nullptr, QString name = QString());
    virtual ~Module();

    static QPointF gridSpacing();
    static float gridSpacingX();
    static float gridSpacingY();
    static float snapX(float x);
    static float snapY(float y);

    QRectF boundingRect() const override; // re-implemented
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override; // re-implemented
    ///\todo this functionality is unnecessary, push functionality to port
    QPointF portPos(const Port *port) const;
    void setStatus(Module::Status status);
    void setStatusText(QString text, int prio);
    void setInfo(QString text, int type);
    void clearMessages();
    void moduleMessage(int type, QString message);
    QList<Message> &messages();
    void setMessagesVisibility(bool visible);
    bool messagesVisible() const;
    bool isOutputStreaming() const;

    void addPort(const vistle::Port &port);
    void removePort(const vistle::Port &port);
    QList<Port *> inputPorts() const;
    QList<Port *> outputPorts() const;
    enum PortKind { Input, Output, Parameter };
    QList<Port *> ports(PortKind kind) const;

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
    int layer() const;
    void setLayer(int layer);
    void updateLayer();

    Port *getGuiPort(const vistle::Port *port) const;
    const vistle::Port *getVistlePort(Port *port) const;

    DataFlowNetwork *scene() const;

    static QColor hubColor(int hub);
signals:
    void createModuleCompound();
    void selectConnected(int direction, int id, QString port = QString());
    void visibleChanged(bool visible);
    void callshowErrorInMainThread();
    void outputStreamingChanged(bool enable);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void updatePosition(QPointF newPos) const;

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

public slots:
    void cloneModule();
    void cloneModuleLinked();
    void restartModule();
    void moveToHub(int hub);
    void replaceWith(QString moduleName);
    void execModule();
    void cancelExecModule();
    void deleteModule();
    void attachDebugger();
    void replayOutput();
    void setOutputStreaming(bool enable);
    void projectToGrid();
    void setParameterDefaults();
    void showError();

private:
    void createGeometry();
    void createActions();
    void createMenus();
    void updateText();
    void doLayout();
    void sendSpawn(int hub, const std::string &module, vistle::message::Spawn::ReferenceType type);
    void setToolTip(QString text);

    QMenu *m_moduleMenu = nullptr;
    QAction *m_selectUpstreamAct = nullptr, *m_selectDownstreamAct = nullptr, *m_selectConnectedAct = nullptr;
    QAction *m_deleteThisAct = nullptr, *m_deleteSelAct = nullptr;
    QAction *m_attachDebugger = nullptr;
    QAction *m_replayOutput = nullptr;
    QAction *m_toggleOutputStreaming = nullptr;
    QAction *m_execAct = nullptr;
    QAction *m_cancelExecAct = nullptr;
    QAction *m_restartAct = nullptr;
    QAction *m_moveToAct = nullptr;
    QMenu *m_moveToMenu = nullptr;
    QMenu *m_replaceWithMenu = nullptr;
    QAction *m_createModuleGroup = nullptr;
    QAction *m_cloneModule = nullptr;
    QAction *m_cloneModuleLinked = nullptr;
    QMenu *m_layerMenu = nullptr;
    QMenu *m_advancedMenu = nullptr;


    int m_hub;
    int m_id;
    int m_clonedFromId = -1;
    boost::uuids::uuid m_spawnUuid;

    ///\todo add data structure for the module information
    QString m_name;
    QString m_displayName;
    Module::Status m_Status;
    QString m_statusText;
    QString m_info;
    QString m_tooltip;
    bool m_errorState = false;
    QList<Message> m_messages;
    bool m_messagesVisible = true;
    bool m_validPosition = false;
    int m_layer = 0;

    QList<Port *> m_inPorts, m_outPorts, m_paramPorts;
    QColor m_color;
    qreal m_fontHeight;
    std::map<vistle::Port, Port *> m_vistleToGui;

    QColor m_borderColor;
};

template<class T>
void Module::setParameter(QString name, const T &value) const
{
    DataFlowNetwork::setParameter<T>(id(), name, value);
}

template<class T>
std::shared_ptr<vistle::ParameterBase<T>> Module::getParameter(QString name) const
{
    return DataFlowNetwork::getParameter<T>(id(), name);
}

} //namespace gui

#endif // VMODULE_H
