#ifndef VCONSTS_H
#define VCONSTS_H

#include <QGraphicsItem>

namespace gui {

enum GraphicsType { DEFAULT,
					MAIN,
					INPUT,
					OUTPUT,
					PARAMETER };

enum ModuleStatus { INITIALIZED,	// green
				    KILLED,			// red
					BUSY,			// yellow
					ERROR };		// black

enum ItemType { TypeModuleItem = QGraphicsItem::UserType + 1,
                TypeArrowItem = QGraphicsItem::UserType + 2,
                TypePortItem = QGraphicsItem::UserType + 3 };

} //namespace gui
#endif // VCONSTS_H
