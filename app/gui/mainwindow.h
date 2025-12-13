#ifndef VISTLE_GUI_MAINWINDOW_H
#define VISTLE_GUI_MAINWINDOW_H

#include <QObject>
#include <QList>
#include <QMainWindow>

namespace gui {

class Parameters;
class ModuleView;
class DataFlowView;
class ModuleBrowser;
class VistleConsole;

namespace Ui {
class MainWindow;

} //namespace Ui

class MainWindow: public QMainWindow {
    friend class UiController;
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QMenu *createPopupMenu();

    QToolBar *toolBar() const;
    QAction *layerWidgetPosition() const;

    QDockWidget *consoleDock() const;
    QDockWidget *moduleViewDock() const;
    QDockWidget *modulesDock() const;
    Parameters *parameters() const;
    ModuleView *moduleView() const;
    DataFlowView *dataFlowView() const;
    VistleConsole *console() const;
    ModuleBrowser *moduleBrowser() const;
    void setQuitOnExit(bool qoe);
    bool isSnapToGrid() const;
    void setInteractionEnabled(bool enable);

public slots:
    void setFilename(const QString &filename);
    void setModified(bool state);
    void newHub(int hub, const QString &hubName, int nranks, const QString &address, int port, const QString &logname,
                const QString &realname, bool hasUi, const QString &systype, const QString &arch, const QString &info,
                const QString &version);
    void deleteHub(int hub);
    void moduleAvailable(int hub, const QString &module, const QString &path, const QString &category,
                         const QString &description);
    void enableConnectButton(bool state);
    void enableSnapToGrid(bool snap);

signals:
    void quitRequested(bool &allowed);
    void newDataFlow();
    void loadDataFlowOnGui();
    void loadDataFlowOnHub();
    void saveDataFlow();
    void saveDataFlowOnGui();
    void saveDataFlowOnHub();
    void executeDataFlow();
    void connectVistle();
    void showSessionUrl();
    void copySessionUrl();
    void selectAllModules();
    void selectInvert();
    void selectClear();
    void selectSourceModules();
    void selectSinkModules();
    void deleteSelectedModules();
    void zoomOrig();
    void zoomAll();
    void aboutQt();
    void aboutVistle();
    void snapToGridChanged(bool snap);

protected:
    void closeEvent(QCloseEvent *);

private:
    Ui::MainWindow *ui = nullptr;
    VistleConsole *m_console = nullptr;
    Parameters *m_parameters = nullptr;
    ModuleBrowser *m_moduleBrowser = nullptr;

    QList<QString> loadModuleFile();

    void readSettings();
    void writeSettings();
};

} //namespace gui
#endif
