#ifndef DATAFLOWVIEW_H
#define DATAFLOWVIEW_H

#include <QGraphicsView>

class QMenu;

namespace gui {

class DataFlowNetwork;
class Module;

class DataFlowView: public QGraphicsView {
    Q_OBJECT

public:
    explicit DataFlowView(QWidget *parent = 0);
    ~DataFlowView();
    static DataFlowView *the();

    DataFlowNetwork *scene() const;
    void contextMenuEvent(QContextMenuEvent *event);
    QList<Module *> selectedModules();
    void setScene(QGraphicsScene *scene);

signals:
    void executeDataFlow();

public slots:
    void enableActions();

    void execModules();
    void cancelExecModules();
    void deleteModules();
    void selectAllModules();

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

    void createActions();
    void createMenu();

    QMenu *m_contextMenu;
    QAction *m_deleteAct;
    QAction *m_execAct;

private:
    static DataFlowView *s_instance;
};

} // namespace gui
#endif // DATAFLOWVIEW_H
