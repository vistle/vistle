#include "uicontroller.h"

namespace gui {

UiController::UiController(vistle::VistleConnection *conn, VistleObserver *observer, QObject *parent)
: QObject(parent)
, m_vistleConnection(conn)
, m_observer(observer)
{
}

} // namespace gui
