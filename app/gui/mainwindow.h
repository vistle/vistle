#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QList>
#include <QMainWindow>

namespace gui {

class Parameters;
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

    QDockWidget *consoleDock() const;
    QDockWidget *parameterDock() const;
    QDockWidget *modulesDock() const;
    Parameters *parameters() const;
    DataFlowView *dataFlowView() const;
    VistleConsole *console() const;
    ModuleBrowser *moduleBrowser() const;
    void setQuitOnExit(bool qoe);

public slots:
    void setFilename(const QString &filename);
    void setModified(bool state);
    void newHub(int hub, const QString &hubName, int nranks, const QString &address, const QString &logname,
                const QString &realname);
    void deleteHub(int hub);
    void moduleAvailable(int hub, const QString &module, const QString &path, const QString &description);
    void enableConnectButton(bool state);

signals:
    void quitRequested(bool &allowed);
    void newDataFlow();
    void loadDataFlow();
    void saveDataFlow();
    void saveDataFlowAs();
    void executeDataFlow();
    void connectVistle();
    void showSessionUrl();
    void copySessionUrl();
    void selectAllModules();
    void deleteSelectedModules();
    void aboutQt();
    void aboutVistle();

protected:
    void closeEvent(QCloseEvent *);

private:
    Ui::MainWindow *ui;
    VistleConsole *m_console;
    Parameters *m_parameters;
    ModuleBrowser *m_moduleBrowser;

    QList<QString> loadModuleFile();

    void readSettings();
    void writeSettings();
};

} //namespace gui
#endif // MAINWINDOW_H
