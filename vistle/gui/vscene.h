#ifndef VSCENE_H
#define VSCENE_H

#include <QGraphicsScene>
#include <QMessageBox>
#include <QVector>
#include "vmodule.h"
#include "varrow.h"
#include "vslot.h"
#include "vconsts.h"

class VScene : public QGraphicsScene
{
public:
    //\TODO how to utilize this?
    enum Mode { InsertModule, InsertLine, MoveItem };
    VScene(QObject *parent = 0);
    ~VScene();
    void addModule(QString modName, QPointF dropPos);
    void removeModule(VModule *mod);
    void sortModules();
    void invertModules();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);      // re-implemented
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);       // re-implemented
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);    // re-implemented

private:
    QList<VModule *> moduleList;                                // list of modules
    Mode mode;                                                  // type of interaction with the scene. Refer to enum
    QGraphicsLineItem *myLine = nullptr;                        // the intermediate line drawn between modules
    QColor myLineColor;                                         // color of the line
    QPointF vLastPoint;                                         // intermediate previous point for arrow drawing
    bool vMouseClick;                                           // boolean for keeping track of clicking
    VSlot *startSlot;                                           // starting slot for module connection
    VSlot *endSlot;                                             // starting slot for module connection
    VModule *startModule = nullptr;                             // starting module for making connection
    VModule *endModule = nullptr;                               // ending module for making connection

    // data for sorting
    QMap<QString, VModule *> sortMap;
    int recSortModules(VModule *parent, int width, int height);

};

#endif // VSCENE_H
