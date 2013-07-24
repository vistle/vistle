#include "vslot.h"

VSlot::VSlot(QPolygonF item, int slot, QGraphicsItem *parent) : QGraphicsPolygonItem(item, parent)
{
    // set the slot type
    slotType = slot;
    QString toolTip;
    switch (slotType) {
        case V_INPUT:
            toolTip = "Input!";
            break;
        case V_OUTPUT:
            toolTip = "Output!";
            break;
        case V_PARAMETER:
            toolTip = "Parameter!";
            break;
        default:
            toolTip = "error!";
            break;
    }

    setToolTip(toolTip);
}

VSlot::VSlot() : QGraphicsPolygonItem()
{
    ///\todo should any functionality be done here?
}
