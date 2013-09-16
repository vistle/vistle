#include "dataflowview.h"
#include "modulebrowser.h"
#include "dataflownetwork.h"

#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>

namespace gui {

DataFlowView::DataFlowView(QWidget *parent)
   : QGraphicsView(parent)
{
}

DataFlowNetwork *DataFlowView::scene() const
{
   return dynamic_cast<DataFlowNetwork *>(QGraphicsView::scene());
}

/*!
 * \brief DataFlowView::dragEnterEvent
 * \param event
 *
 * \todo clean up the distribution of event handling.
 */
void DataFlowView::dragEnterEvent(QDragEnterEvent *e)
{
   const QMimeData *mimeData = e->mimeData();
   const QStringList &mimeFormats = mimeData->formats();
   if (mimeFormats.contains(ModuleBrowser::mimeFormat())) {
      e->acceptProposedAction();
   }
}

void DataFlowView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

/*!
 * \brief DataFlowView::dropEvent takes action on a drop event anywhere in the window.
 *
 * This drop event method is currently the only drop event in the program. It looks for drops into the
 * QGraphicsView (drawArea), and calls the scene to add a module depending on the position.
 *
 * \param event
 * \todo clarify all the event handling, and program the creation and handling of events more elegantly.
 * \todo put information into the event, to remove the need to have drag events in the mainWindow
 */
void DataFlowView::dropEvent(QDropEvent *event)
{
    QPointF newPos = mapToScene(event->pos());

    if (event->mimeData()->formats().contains(ModuleBrowser::mimeFormat())) {

        QByteArray encoded = event->mimeData()->data(ModuleBrowser::mimeFormat());
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        while (!stream.atEnd())
        {
           QString moduleName;
           stream >> moduleName;
           scene()->addModule(moduleName, newPos);
        }
    }
}



} // namespace gui
