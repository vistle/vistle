#include "vscene.h"

/*!
 * \brief VScene::VScene
 * \param parent
 */
VScene::VScene(QObject *parent) : QGraphicsScene(parent)
{
    // Initialize starting scene information.
    mode = InsertLine;
    myLineColor = Qt::black;
    vMouseClick = false;
}

/*!
 * \brief VScene::~VScene
 */
VScene::~VScene()
{
    moduleList.clear();
    sortMap.clear();
}

/*!
 * \brief VScene::addModule add a module to the draw area.
 * \param modName
 * \param dropPos
 */
void VScene::addModule(QString modName, QPointF dropPos)
{
    VModule *module = new VModule(0, modName);
    ///\todo improve how the data such as the name is set in the module.
    addItem(module);
    module->setPos(dropPos);
    module->setStatus(V_ACTIVE);

    ///\todo add the objects to a data structure. This should be done in the scene,
    // where various event handling happens. For now perhaps use a simple vector. Look
    // at the Qt sample implementations for more inspiration, and a better implementation.
    moduleList.append(module);
}

/*!
 * \brief VScene::removeModule search for and remove a module from the data structures.
 * \param mod
 */
void VScene::removeModule(VModule *mod)
{


}

/*!
 * \brief VScene::sortModules
 */
void VScene::sortModules()
{
    QStringList dimList;
    VModule *mod;
    qreal minX;
    qreal minY;

    // for now simply create a 2d vector of VModules (done in the header). Add one row to the vector to start
    //QList<VModule *> row;
    // the number of origin modules
    int height = 0;
    // this is a cop-out, but keep for now: the number of rows in the vector is the same as the largest
    //  possible dimensions, the number of modules in the scene.
    /*for (int i = 0; i <= moduleList.count(); i++) {
        sortVec.append(row);
    }*/

    // Here are the steps of the program:
    // Find the origin modules, at the moment by looping through the list of modules created in addModule.
    //  recursively follow the module's outputs, assigning levels and flags to each module.

    sortMap.clear();
    foreach (VModule *module, moduleList) {
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

    ///\todo test this, perhaps find a better alternative, so that the view pane does not increase in width.
    views().first()->centerOn(minX, minY);
    clearSelection();
}

/*!
 * \brief VScene::recSortModules recursively follow a module's children, determining and setting positions
 * \param parent
 * \param width
 * \param height
 */
int VScene::recSortModules(VModule *parent, int width, int height)
{
    int h = 0;
    QStringList dimList;
    QString str;

    // The algorithm for recursive sort currently works, but it has a number of bugs,
    //  including a module migrating upon each successive sort, and the width of the pane increasing
    //  with each successive sort.
    str = QString::number(width) + "," + QString::number(height);
    if (sortMap.contains(str)) {
        // if the dimensions are already occupied, the parent needs to shift dimensions.
        // find the parent that occupies the x-1 key and shift it y+1
        foreach (VModule *module, parent->getParentModules()) {
            if (module->getKey().contains(QString::number(width - 1) + ",")) {
                sortMap.remove(module->getKey());
                dimList = module->getKey().split(",", QString::SkipEmptyParts, Qt::CaseInsensitive);
                module->unsetVisited();
                recSortModules(module, dimList[0].toInt(), dimList[1].toInt() + 1);

                return 0;
            }
        }

        //sortMap.remove(parent->parentModules)

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
                foreach (VModule *module, parent->getChildModules()) {
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
            foreach (VModule *module, parent->getChildModules()) {
                recSortModules(module, width + 1, height + h);
                h++;
            }
        }
    }

    return width;
}

/*!
 * \brief VScene::invertModules inverts the orientation of all the modules in the scene.
 *
 * This method inverts the orientation of the modules in the scene. This is most easily done by simply
 * changing the location of each slot, rather than actually swapping the x and y coordinates.
 */
void VScene::invertModules()
{

}

///
///\todo an exception is thrown upon a simple click inside a module's slot.
///\todo left clicking inside a module's context menu still sends the left click event to the scene, but
///      the way it is programmed creates a segfault.
///

/*!
 * \brief VScene::mousePressEvent
 * \param event
 *
 * \todo write proper documentation for this section
 * \todo test arrow drawing and unforseen events more thoroughly
 */
void VScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // ignore all buttons but the left button
    ///\todo add other button support
    //if (event->button() != Qt::LeftButton) { return; }
    if (event->button() == Qt::RightButton) {
        ///\todo open a context menu?
    }

    // store the click location
    vLastPoint = event->scenePos();
    // set the click flag
    vMouseClick = true;
    int slotType;

    // If the user clicks on a module, test for what is being clicked on.
    //  If okay, begin the process of drawing a line.
    // See what has been selected from an object, if anything.
    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    ///\todo add other objects and dynamic cast checks here
    ///\todo dynamic cast is not a perfect solution, is there a better one?
    if (item) {
        if (item->type() == TypeVSlotItem) {
            startSlot = dynamic_cast<VSlot *>(item);
            // Get the type of slot that was clicked on
            //startSlot = module->getSlot(event->scenePos(), slotType);
            // Test for slot type
            switch (startSlot->slot()) {
                case V_INPUT:
                case V_OUTPUT:
                case V_PARAMETER:
                    myLine = new QGraphicsLineItem(QLineF(event->scenePos(),
                                                          event->scenePos()));
                    myLine->setPen(QPen(myLineColor, 2));
                    addItem(myLine);
                    startModule = dynamic_cast<VModule *>(startSlot->parentItem());
                    break;
                case V_MAIN:
                    //The object inside the slots has been clicked on
                    ///\todo add functionality
                    break;
                case V_DEFAULT:
                    // Another part of the object has been clicked, ignore
                    break;
            }
        }
    }

    QGraphicsScene::mousePressEvent(event);
}

/*!
 * \brief VScene::mouseReleaseEvent watches for click events
 * \param event
 */
void VScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    int slotType;
    bool arrowDrawFlag = false;
    QGraphicsItem *item;
    // if there was a click
    if ((vMouseClick) && (event->scenePos() == vLastPoint)) {
        ///\todo what functionality to add?
        item = itemAt(event->scenePos(), QTransform());
        if (item) {
            if (item->type() == TypeVArrowItem) {
                // delete arrow
                VArrow *arrow = dynamic_cast<VArrow *>(item);
                // remove the modules from any lists. Need to find which list the modules belong to
                //  and then remove the child and parent (or param) from both locations.
                VModule *stMod = arrow->startItem();
                VModule *endMod = arrow->endItem();
                if (arrow->connectionType() == V_OUTPUT) {
                    if (!(stMod->removeChild(endMod))) { qDebug()<<"Fail!"; }
                    if (!(endMod->removeParent(stMod))) { qDebug()<<"Fail!"; }

                } else if (arrow->connectionType() == V_PARAMETER) {
                    stMod->removeParameter(endMod);
                    endMod->removeParameter(stMod);
                } else {
                    //something has gone horribly wrong
                }
                arrow->startItem()->removeArrow(arrow);
                arrow->endItem()->removeArrow(arrow);
                delete arrow;
            }
        }
    } else if (vMouseClick && myLine) {

        // clean up arrow
        removeItem(myLine);
        delete myLine;
        myLine = nullptr;

        // Begin testing for the finish of the line draw.
        item = itemAt(event->scenePos(), QTransform());
        if (item) {
            if (item->type() == TypeVSlotItem) {
                endSlot = dynamic_cast<VSlot *>(item);
                // Get the type of slot that was clicked on
                //endSlot = module->getSlot(event->scenePos(), slotType);
                // Test over the possibilities
                // this functionality is very basic for the moment
                ///\todo improve testing for connection compatibility
                switch (endSlot->slot()) {
                    case V_INPUT:
                        if (startSlot->slot() == V_OUTPUT) {
                            endModule = dynamic_cast<VModule *>(endSlot->parentItem());
                            if (startModule != endModule) {
                                VArrow *arrow = new VArrow(startModule, endModule, startSlot, endSlot, true, V_OUTPUT);
                                arrow->setColor(myLineColor);
                                startModule->addArrow(arrow);
                                endModule->addArrow(arrow);
                                startModule->addChild(endModule);
                                endModule->addParent(startModule);
                                arrow->setZValue(-1000.0);
                                addItem(arrow);
                                arrow->updatePosition();
                            }
                        }
                        break;
                    case V_OUTPUT:
                        if (startSlot->slot() == V_INPUT) {
                            endModule = dynamic_cast<VModule *>(endSlot->parentItem());
                            if (startModule != endModule) {
                                VArrow *arrow = new VArrow(endModule, startModule, endSlot, startSlot, true, V_OUTPUT);
                                arrow->setColor(myLineColor);
                                startModule->addArrow(arrow);
                                endModule->addArrow(arrow);
                                endModule->addChild(startModule);
                                startModule->addParent(endModule);
                                arrow->setZValue(-1000.0);
                                addItem(arrow);
                                arrow->updatePosition();
                            }
                        }
                        break;
                    case V_PARAMETER:
                        if (startSlot->slot() == V_PARAMETER) {
                            endModule = dynamic_cast<VModule *>(endSlot->parentItem());
                            if (startModule != endModule) {
                                VArrow *arrow = new VArrow(startModule, endModule, startSlot, endSlot, false, V_PARAMETER);
                                arrow->setColor(myLineColor);
                                startModule->addArrow(arrow);
                                endModule->addArrow(arrow);
                                startModule->addParameter(endModule);
                                endModule->addParameter(startModule);
                                arrow->setZValue(-1000.0);
                                addItem(arrow);
                                arrow->updatePosition();
                            }
                        }
                        break;
                    case V_MAIN:
                    case V_DEFAULT:
                    case V_ERROR:
                        ///\todo what to do here?
                        break;
                    default:
                        // something has gone horribly wrong
                        break;
                }

                ///\todo moved this functionality to the switch, do I need to leave anything here?
                if (arrowDrawFlag && startModule != endModule)
                {
                    ///\todo does anything need to be cleaned up?
                }
            }
        }
    }

    // Clear data
    startModule = nullptr;
    endModule = nullptr;
    startSlot = nullptr;
    endSlot = nullptr;
    vMouseClick = false;
    QGraphicsScene::mouseReleaseEvent(event);
}

/*!
 * \brief VScene::mouseMoveEvent
 * \param event
 */
void VScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    ///\todo should additional tests be present here?
    if (mode == InsertLine
        && myLine != 0
        && (startSlot->slot() >= V_INPUT)) {
        QLineF newLine(myLine->line().p1(), event->scenePos());
        myLine->setLine(newLine);
    } else {
        QGraphicsScene::mouseMoveEvent(event);
    }
}
