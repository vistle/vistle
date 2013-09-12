#ifndef UICONTROLLER_H
#define UICONTROLLER_H

#include <QObject>

#include <userinterface/vistleconnection.h>
#include "vistleobserver.h"

namespace gui {

class UiController : public QObject
{
   Q_OBJECT

public:
   explicit UiController(vistle::VistleConnection *conn, VistleObserver *observer, QObject *parent = 0);

signals:

public slots:

private:
    vistle::VistleConnection *m_vistleConnection = nullptr;
    VistleObserver *m_observer = nullptr;
};

} // namespace gui
#endif // UICONTROLLER_H
