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

#include "consts.h"
#include "port.h"

namespace gui {

class Arrow;

class Module : public QObject, public QGraphicsPolygonItem
{
    Q_OBJECT

signals:
    void mouseClickEvent();

public:
    Module(QGraphicsItem *parent = 0, QString name = 0);
    virtual ~Module();
    QRectF boundingRect() const;                        // re-implemented
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget);                        // re-implemented
    Port *getPort(QPointF pos, int &portType);
    QPolygonF polygon() const
        { return baseShape; }
    ///\todo rename Arrow to Connection
    void addArrow(Arrow *arrow);
    void removeArrow(Arrow *arrow);
    void clearArrows();
    int type() const { return TypeModuleItem; }
    ///\todo this functionality is unnecessary, push functionality to port
    QPointF portPos(Port *port);
    void setStatus(ModuleStatus status) { myStatus = status; }

    void addParent(Module *parentMod) { parentModules.append(parentMod); }
    void addChild(Module *childMod) { childModules.append(childMod); }
    ///\todo change name to connectParam
    void addParameter(Module *paramMod) { paramModules.append(paramMod); }
    bool removeParent(Module *parentMod);
    bool removeChild (Module *childMod);
    ///\todo change name to disconnectParam
    bool removeParameter(Module *paramMod);
    ///\todo put in map
    void setVisited() { modIsVisited = true; }
    void unsetVisited() { modIsVisited = false; }
    bool isVisited() { return modIsVisited; }

    QList<Module *> getChildModules() { return childModules; }
    QList<Module *> getParentModules() { return parentModules; }
    QList<Module *> getParamModules() { return paramModules; }
    void setKey(QString key) { myKey = key; }
    QString getKey() { return myKey; }
    void clearKey() { myKey = QString(""); }

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

public slots:
    void copy();
    void deleteModule();
    void deleteConnections();

private:
    void createSlots();
    void createActions();
    void createMenus();
    QMenu *moduleMenu;
    QAction *copyAct;
    QAction *deleteAct;
    QAction *deleteConnectionAct;

    qreal w;                                            // width of the module
    qreal h;                                            // height of the module
    int xAddr;                                          // important calculated value for the x magnitude
    int yAddr;                                          // important calculated value for the y magnitude
    bool vMouseClick;                                   // boolean for keeping track of whether a click was made
    QSizeF size;                                        // size of the module
    QList<Port *> portList;                             // list of all the ports in the module
    QPointF vLastPoint;                                 // point for keeping track of whether a click was made
    QPolygonF baseShape;                                // base polygon of the module
    QGraphicsPolygonItem *statusShape;
    QList<Arrow *> arrowList;                           // list of arrows connected to the module

    QList<Module *> parentModules;
    QList<Module *> childModules;
    QList<Module *> paramModules;
    bool modIsVisited;
    ///\todo remove the use of myKey in the module, along with any visited bools -- 
    /// these can be implemented in the map itself, and leave the module out of it.
    QString myKey;

    /** \todo add data structure for the module information */
    QString modName;
    ModuleStatus myStatus;

};

} //namespace gui

#endif // VMODULE_H
