/*********************************************************************************/
/** \file port.cpp
 *
 * simple re-implementation of QGraphicsPolygonItem
 */
/**********************************************************************************/
#include "port.h"

namespace gui {

Port::Port(QPolygonF item, GraphicsType port, QGraphicsItem *parent) : QGraphicsPolygonItem(item, parent)
{
    // set the port type
    portType = port;
    QString toolTip;
    switch (portType) {
        case INPUT:
            toolTip = "Input!";
            break;
        case OUTPUT:
            toolTip = "Output!";
            break;
        case PARAMETER:
            toolTip = "Parameter!";
            break;
        default:
            toolTip = "Error!";
            break;
    }

    setToolTip(toolTip);
}

Port::Port() : QGraphicsPolygonItem()
{
}

} //namespace gui
