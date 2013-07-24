#ifndef VCONSTS_H
#define VCONSTS_H

//\TODO should more includes go here?
#include <QGraphicsItem>

// Define consts for use
#define NULL 0

#define V_ERROR -1
#define V_DEFAULT 0
#define V_MAIN 1
#define V_INPUT 2
#define V_OUTPUT 3
#define V_PARAMETER 4

#define V_ACTIVE 5
#define V_WAITING 6
#define V_PASSIVE 7

enum ItemType { TypeVModuleItem = QGraphicsItem::UserType + 1,
                TypeVArrowItem = QGraphicsItem::UserType + 2,
                TypeVSlotItem = QGraphicsItem::UserType + 3 };

#endif // VCONSTS_H
