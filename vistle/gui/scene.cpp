/*********************************************************************************/
/** \file scene.cpp
 *
 * The brunt of the graphical processing and UI interaction happens here --
 * - modules and connections are created and modified
 * - the brunt of the event handling is done
 * - any interactions between modules, such as sorting, is performed
 */
/**********************************************************************************/
#include "scene.h"

#include <userinterface/userinterface.h>
#include <core/message.h>

#include <QGraphicsView>
#include <QStringList>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

namespace gui {

/*!
 * \brief Scene::Scene
 * \param parent
 */
Scene::Scene(QObject *parent) : QGraphicsScene(parent)
{
    // Initialize starting scene information.
    mode = InsertLine;
    m_LineColor = Qt::black;
    vMouseClick = false;
}

/*!
 * \brief Scene::~Scene
 */
Scene::~Scene()
{
    moduleList.clear();
    sortMap.clear();
}

void Scene::setModules(QList<QString> moduleNameList)
{
    m_ModuleNameList = moduleNameList;
}

void Scene::setRunner(VistleConnection *runner)
{
    m_Runner = runner;
}

/*!
 * \brief Scene::addModule add a module to the draw area.
 * \param modName
 * \param dropPos
 */
void Scene::addModule(QString modName, QPointF dropPos)
{
    Module *module = new Module(0, modName);
    ///\todo improve how the data such as the name is set in the module.
    addItem(module);
    module->setPos(dropPos);
    module->setStatus(SPAWNING);

    vistle::message::Spawn spawnMsg(0, modName.toUtf8().constData());
    module->setSpawnUuid(spawnMsg.uuid());
    m_Runner->sendMessage(spawnMsg);

    ///\todo add the objects only to the map (sortMap) currently used for sorting, not to the list.
    ///This will remove the need for moduleList altogether
    moduleList.append(module);
}

void Scene::addModule(int moduleId, const boost::uuids::uuid &spawnUuid, QString name)
{
   std::cerr << "addModule: name=" << name.toStdString() << ", id=" << moduleId << std::endl;
   Module *mod = findModule(spawnUuid);
   if (!mod)
      mod = findModule(moduleId);
   if (!mod) {
      mod = new Module(nullptr, name);
      addItem(mod);
      mod->setStatus(SPAWNING);
      moduleList.append(mod);
   }

   mod->setId(moduleId);
   mod->setName(name+"_"+QString::number(moduleId));
}

Module *Scene::findModule(int id) const
{
   for (Module *mod: moduleList) {
      if (mod->id() == id) {
         return mod;
      }
   }

   return nullptr;
}

Module *Scene::findModule(const boost::uuids::uuid &spawnUuid) const
{
   for (Module *mod: moduleList) {
      if (mod->spawnUuid() == spawnUuid) {
         return mod;
      }
   }

   return nullptr;
}

/*!
 * \brief Scene::removeModule search for and remove a module from the data structures.
 * \param mod
 *
 * \todo add functionality
 */
void Scene::removeModule(Module *mod)
{


}

/*!
 * \brief Scene::sortModules top level of sorting modules.
 *
 * The sort is performed on two levels -- the beginning of the sort, and the actual recursion.
 * The modules first must be looped through. This is done by iterating over the moduleList (which is just a list of modules
 * in order of their creation), looking for modules that are "origins" -- modules with no parents.
 * For each origin module, call the recursive sort.
 * After each module is sorted and assigned a key corresponding to its relative location in the map, pull the dimensions out of
 * the key and set a real location in the scene for it.
 *
 * The algorithm has a number of bugs:
 * \todo when there are large numbers of modules, a single module will "float" in strange directions upon each successive sort
 * \todo when the sort button is pressed multiple times, the width of the pane increases.
 */
void Scene::sortModules()
{
    QStringList dimList;
    Module *mod;
    qreal minX;
    qreal minY;

    // height, equal initially to the number of origin modules
    int height = 0;

    sortMap.clear();
    foreach (Module *module, moduleList) {
        //look to see if a module is an origin module.
        if (module->getParentModules().isEmpty()) {
            recSortModules(module, 0, height);
            height++;
        }
    }

    // get the scene's current rectangular dimension, use it to set the module's positions
    minX = sceneRect().topLeft().x() + 20;
    minY = sceneRect().topLeft().y() + 20;

    for (auto i = sortMap.cbegin(); i != sortMap.cend(); i++) {
        // dereference the iterator
        mod = *i;
        // get the dimensions from the key
        dimList = i.key().split(",", QString::SkipEmptyParts, Qt::CaseInsensitive);

        // set the dimensions of the module
        mod->setPos(QPointF((dimList[0].toInt() * 250) + minX, (dimList[1].toInt() * 100) + minY));

        mod->unsetVisited();
        mod->setToolTip(i.key());
        mod->clearKey();
    }

    // test this and find a better alternative, so that the view pane does not increase in width.
    views().first()->centerOn(minX, minY);
    clearSelection();
}

/*!
 * \brief Scene::recSortModules recursively follow a module's children, determining and setting positions
 * \param parent
 * \param width
 * \param height
 */
int Scene::recSortModules(Module *parent, int width, int height)
{
    int h = 0;
    QStringList dimList;
    QString str;

    str = QString::number(width) + "," + QString::number(height);
    if (sortMap.contains(str)) {
        // if the dimensions are already occupied, the parent needs to shift dimensions.
        // find the parent that occupies the x-1 key and shift it y+1
        foreach (Module *module, parent->getParentModules()) {
            if (module->getKey().contains(QString::number(width - 1) + ",")) {
                sortMap.remove(module->getKey());
                dimList = module->getKey().split(",", QString::SkipEmptyParts, Qt::CaseInsensitive);
                module->unsetVisited();
                recSortModules(module, dimList[0].toInt(), dimList[1].toInt() + 1);

                return 0;
            } // end if
        } // end foreach
    } else {
        // test to see if the module was visited, and if the new x is larger than the previous. If so, re-add
        //  it to the map with the new coordinates. Recursion continues as before, and children should also
        //  get the correct coordinates.
        ///\todo test this
        if (parent->isVisited()) {
            dimList = parent->getKey().split(",", QString::SkipEmptyParts, Qt::CaseInsensitive);
            if (width > dimList[0].toInt()) {
                sortMap.remove(parent->getKey());
                sortMap[str] = parent;
                parent->setKey(str);

                // loop through the children, recursively setting the position for each
                foreach (Module *module, parent->getChildModules()) {
                    recSortModules(module, width + 1, height + h);
                    h++;
                }
            }
        // standard behavior. Simply add the module to the map
        } else {
            sortMap[str] = parent;
            parent->setKey(str);
            parent->setVisited();

            // loop through the children, recursively setting the position for each
            foreach (Module *module, parent->getChildModules()) {
                recSortModules(module, width + 1, height + h);
                h++;
            } //end foreach
        } // end else
    } // end else

    return width;
}

/*!
 * \brief Scene::invertModules inverts the orientation of all the modules in the scene.
 *
 * This method will invert the orientation of the modules in the scene. This is most easily done by simply
 * changing the location of each port, rather than actually swapping the x and y coordinates.
 * \todo implement this
 */
void Scene::invertModules()
{

}

///\todo an exception is very occasionally thrown upon a simple click inside a module's port.
///\todo left clicking inside a module's context menu still sends the left click event to the scene, but creates
///a segfault when the connection is completed

/*!
 * \brief Scene::mousePressEvent
 * \param event
 *
 * \todo test connection drawing and unforseen events more thoroughly
 */
void Scene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    ///\todo add other button support
    if (event->button() == Qt::RightButton) {
        ///\todo open a context menu?
    }

    // store the click location
    vLastPoint = event->scenePos();
    // set the click flag
    vMouseClick = true;

    // If the user clicks on a module, test for what is being clicked on.
    //  If okay, begin the process of drawing a line.
    // See what has been selected from an object, if anything.
    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    startPort = dynamic_cast<gui::Port *>(item);
    ///\todo add other objects and dynamic cast checks here
    ///\todo dynamic cast is not a perfect solution, is there a better one?
    if (startPort) {
       // Test for port type
       switch (startPort->port()) {
          case INPUT:
          case OUTPUT:
          case PARAMETER:
             m_Line = new QGraphicsLineItem(QLineF(event->scenePos(),
                      event->scenePos()));
             m_Line->setPen(QPen(m_LineColor, 2));
             addItem(m_Line);
             startModule = dynamic_cast<Module *>(startPort->parentItem());
             break;
          case MAIN:
             //The object inside the ports has been clicked on
             ///\todo add functionality
             break;
          case DEFAULT:
             // Another part of the object has been clicked, ignore
             break;
       } //end switch
    } //end if (startPort)

    QGraphicsScene::mousePressEvent(event);
}

/*!
 * \brief Scene::mouseReleaseEvent watches for click events
 * \param event
 */
void Scene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem *item;
    // if there was a click
    if ((vMouseClick) && (event->scenePos() == vLastPoint)) {
        item = itemAt(event->scenePos(), QTransform());
        if (item) {
            if (item->type() == TypeConnectionItem) {
                // delete connection
                Connection *connection = dynamic_cast<Connection *>(item);
                // remove the modules from any lists. Need to find which list the modules belong to
                //  and then remove the child and parent (or param) from both locations.
                Module *stMod = connection->startItem();
                Module *endMod = connection->endItem();
                ///\todo add vistle message sending
                if (connection->connectionType() == OUTPUT) {
                    if (!(stMod->removeChild(endMod))) { std::cerr<<"failed to remove child"<<std::endl; }
                    if (!(endMod->removeParent(stMod))) { std::cerr<<"failed to remove parent"<<std::endl; }

                } else if (connection->connectionType() == PARAMETER) {
                    stMod->disconnectParameter(endMod);
                    endMod->disconnectParameter(stMod);
                } else {

                }
                connection->startItem()->removeConnection(connection);
                connection->endItem()->removeConnection(connection);
                delete connection;
            } //end if (item->type() == TypeconnectionItem)
        } //end if (item)
    // if there was not a click, and if m_line already was drawn
    } else if (vMouseClick && m_Line) {

        // clean up connection
        removeItem(m_Line);
        delete m_Line;
        m_Line = nullptr;

        // Begin testing for the finish of the line draw.
        item = itemAt(event->scenePos(), QTransform());
        if (item) {
            if (item->type() == TypePortItem) {
                endPort = dynamic_cast<Port *>(item);
                // Test over the port types
                ///\todo improve testing for viability of connection
                switch (endPort->port()) {
                    case INPUT:
                        if (startPort->port() == OUTPUT) {
                            endModule = dynamic_cast<Module *>(endPort->parentItem());
                            if (startModule != endModule) {
                                Connection *connection = new Connection(startModule, endModule, startPort, endPort, true, OUTPUT);
                                connection->setColor(m_LineColor);
                                startModule->addConnection(connection);
                                endModule->addConnection(connection);
                                startModule->addChild(endModule);
                                endModule->addParent(startModule);
                                connection->setZValue(-1000.0);
                                addItem(connection);
                                connection->updatePosition();
                            }
                        }
                        break;
                    case OUTPUT:
                        if (startPort->port() == INPUT) {
                            endModule = dynamic_cast<Module *>(endPort->parentItem());
                            if (startModule != endModule) {
                                Connection *connection = new Connection(endModule, startModule, endPort, startPort, true, OUTPUT);
                                connection->setColor(m_LineColor);
                                startModule->addConnection(connection);
                                endModule->addConnection(connection);
                                endModule->addChild(startModule);
                                startModule->addParent(endModule);
                                connection->setZValue(-1000.0);
                                addItem(connection);
                                connection->updatePosition();
                            }
                        }
                        break;
                    case PARAMETER:
                        if (startPort->port() == PARAMETER) {
                            endModule = dynamic_cast<Module *>(endPort->parentItem());
                            if (startModule != endModule) {
                                Connection *connection = new Connection(startModule, endModule, startPort, endPort, false, PARAMETER);
                                connection->setColor(m_LineColor);
                                startModule->addConnection(connection);
                                endModule->addConnection(connection);
                                startModule->connectParameter(endModule);
                                endModule->connectParameter(startModule);
                                connection->setZValue(-1000.0);
                                addItem(connection);
                                connection->updatePosition();
                            }
                        }
                        break;
                    case MAIN:
                    case DEFAULT:
                        ///\todo what to do here?
                        break;
                    default:
                        break;
                } //end switch
            } //end if (item->type() == TypePortItem)
        } //end if (item)
    } //end if (vMouseClick && m_Line)

    // Clear data
    startModule = nullptr;
    endModule = nullptr;
    startPort = nullptr;
    endPort = nullptr;
    vMouseClick = false;
    QGraphicsScene::mouseReleaseEvent(event);
}

/*!
 * \brief Scene::mouseMoveEvent
 * \param event
 */
void Scene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!startPort) {
       QGraphicsScene::mouseMoveEvent(event);
       return;
    }
    GraphicsType port = startPort->port();
    ///\todo should additional tests be present here?
    // if correct mode, m_line has been created, and there is a correctly initialized port:
    if (mode == InsertLine
        && m_Line != 0
        && (port == INPUT || port == OUTPUT || port == PARAMETER)) {
        // update the line drawing
        QLineF newLine(m_Line->line().p1(), event->scenePos());
        m_Line->setLine(newLine);
    // otherwise call standard mouse move
    } else {
        QGraphicsScene::mouseMoveEvent(event);
    }
}

} //namespace gui
