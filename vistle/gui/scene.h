#ifndef VSCENE_H
#define VSCENE_H

#include "module.h"
#include "connection.h"
#include "port.h"
#include "consts.h"
#include "handler.h"

#include <QList>
#include <QString>
#include <QGraphicsItem>
#include <QGraphicsScene>

namespace gui {

class Scene : public QGraphicsScene
{
public:
    Scene(QObject *parent = 0);
    ~Scene();
    void addModule(QString modName, QPointF dropPos);
    void removeModule(Module *mod);
    void sortModules();
    void invertModules();
    void setModules(QList<QString> moduleNameList);
    void setRunner(UiRunner *runnner);

    enum Mode { InsertLine, InsertModule };

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);      //< re-implemented
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);       //< re-implemented
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);    //< re-implemented

private:
    QList<Module *> moduleList;                                 //< list of modules
    QList<QString> m_ModuleNameList;
    Mode mode;                                                  //< type of interaction with the scene. Refer to enum
    QGraphicsLineItem *m_Line = nullptr;                        //< the intermediate line drawn between modules
    QColor m_LineColor;                                         //< color of the line
    QPointF vLastPoint;                                         //< intermediate previous point for connection drawing
    bool vMouseClick;                                           //< boolean for keeping track of if a click is made
    Port *startPort;                                            //< starting port for module connection
    Port *endPort;                                              //< starting port for module connection
    Module *startModule = nullptr;                              //< starting module for making connection
    Module *endModule = nullptr;                                //< ending module for making connection

    // data for sorting
    ///\todo remove the qlist of modules, put everything into the QMap initially
    QMap<QString, Module *> sortMap;
    int recSortModules(Module *parent, int width, int height);

    ///\todo push this functionality to vHandler
    UiRunner *m_Runner;

};

} //namespace gui

#endif // VSCENE_H
