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

    bool snapshot(const QString &filename);

signals:
    void executeDataFlow();

public slots:
    void enableActions();

    void execModules();
    void cancelExecModules();
    void deleteModules();
    void selectAllModules();
    void selectSourceModules();
    void selectSinkModules();

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

    void createActions();
    void createMenu();

    QMenu *m_contextMenu = nullptr;
    QAction *m_deleteAct = nullptr;
    QAction *m_execAct = nullptr;
    QAction *m_selectSourcesAct = nullptr;
    QAction *m_selectSinksAct = nullptr;

private:
    static DataFlowView *s_instance;
};

} // namespace gui
#endif // DATAFLOWVIEW_H
