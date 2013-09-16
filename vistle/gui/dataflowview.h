#ifndef DATAFLOWVIEW_H
#define DATAFLOWVIEW_H

#include <QGraphicsView>

namespace gui {

class DataFlowNetwork;

class DataFlowView : public QGraphicsView
{
   Q_OBJECT

public:
   explicit DataFlowView(QWidget *parent = 0);

   DataFlowNetwork *scene() const;

signals:

public slots:

protected:
   void dragEnterEvent(QDragEnterEvent *e);
   void dragMoveEvent(QDragMoveEvent *event);
   void dropEvent(QDropEvent *event);
};

} // namespace gui
#endif // DATAFLOWVIEW_H
