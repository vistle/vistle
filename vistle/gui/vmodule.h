#ifndef VMODULE_H
#define VMODULE_H

#include <QGraphicsPolygonItem>
#include <QObject>
#include <QtGui>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QAction>
#include <QMenu>
#include <vector>
#include "vconsts.h"
#include "vscene.h"
#include "vslot.h"

// resolves circular dependency
class VArrow;

class VModule : public QObject, public QGraphicsPolygonItem
{
    Q_OBJECT

signals:
    void mouseClickEvent();

public:
    VModule(QGraphicsItem *parent = 0, QString name = 0);
    virtual ~VModule();
    QRectF boundingRect() const;                        // re-implemented
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget);                        // re-implemented
    VSlot *getSlot(QPointF pos, int &slotType);
    QPolygonF polygon() const
        { return baseShape; }
    void addArrow(VArrow *arrow);
    void removeArrow(VArrow *arrow);
    void clearArrows();
    int type() const { return TypeVModuleItem; }
    QPointF slotPos(VSlot *slot);
    void setStatus(int status) { myStatus = status; }

    void addParent(VModule *parentMod) { parentModules.append(parentMod); }
    void addChild(VModule *childMod) { childModules.append(childMod); }
    void addParameter(VModule *paramMod) { paramModules.append(paramMod); }
    bool removeParent(VModule *parentMod);
    bool removeChild (VModule *childMod);
    bool removeParameter(VModule *paramMod);
    void setVisited() { modIsVisited = true; }
    void unsetVisited() { modIsVisited = false; }
    bool isVisited() { return modIsVisited; }

    QList<VModule *> getChildModules() { return childModules; }
    QList<VModule *> getParentModules() { return parentModules; }
    QList<VModule *> getParamModules() { return paramModules; }
    void setKey(QString key) { myKey = key; }
    QString getKey() { return myKey; }
    void clearKey() { myKey = QString(""); }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
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
    QList<VSlot *> slotList;                            // list of all the slots in the module
    QPointF vLastPoint;                                 // point for keeping track of whether a click was made
    QPolygonF baseShape;                                // base polygon of the module
    QGraphicsPolygonItem *statusShape;
    QList<VArrow *> arrowList;                          // list of arrows connected to the module
    QGraphicsScene *myScene;                            // the object's scene **does the module need this?**

    QList<VModule *> parentModules;
    QList<VModule *> childModules;
    QList<VModule *> paramModules;
    bool modIsVisited;
    QString myKey;

    /** \todo add data structure for the module information */
    QString modName;
    int myStatus;

};

#endif // VMODULE_H
