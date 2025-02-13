#ifndef VISTLE_GUI_DATAFLOWVIEW_H
#define VISTLE_GUI_DATAFLOWVIEW_H

#include <QGraphicsView>

class QMenu;
class QComboBox;
class QToolBar;

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

    bool isSnapToGrid() const;

    QAction *addToToolBar(QToolBar *toolbar, QAction *before = nullptr);
    int visibleLayer() const;
    int numLayers() const;
    void setNumLayers(int num);

signals:
    void executeDataFlow();
    void visibleLayerChanged(int layer);

public slots:
    void enableActions();
    void changeConnectionEmphasis();

    void execModules();
    void cancelExecModules();
    void deleteModules();
    void selectAllModules();
    void selectInvert();
    void selectClear();
    void selectSourceModules();
    void selectSinkModules();

    void zoomOrig();
    void zoomAll();

    void snapToGridChanged(bool snap);

protected:
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *e);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

    void createActions();
    void createMenu();

    bool m_isPanning = false;
    QPoint m_panPos;
    QCursor m_savedCursor;

    QMenu *m_contextMenu = nullptr;
    QAction *m_deleteAct = nullptr;
    QAction *m_execAct = nullptr;
    QAction *m_selectSourcesAct = nullptr;
    QAction *m_selectSinksAct = nullptr;
    QAction *m_selectInvertAct = nullptr;

    QComboBox *m_visibleLayer = nullptr;
    int m_numLayers = 1;

    bool m_snapToGrid = true;

private:
    void readSettings();
    void writeSettings();
    static DataFlowView *s_instance;
};

} // namespace gui
#endif
