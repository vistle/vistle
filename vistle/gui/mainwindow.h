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

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QDockWidget *consoleDock() const;
    QDockWidget *parameterDock() const;
    QDockWidget *modulesDock() const;
    Parameters *parameters() const;
    DataFlowView *dataFlowView() const;
    VistleConsole *console() const;

public slots:
    void setFilename(const QString &filename);
    void setModified(bool state);
    void moduleAvailable(int hub, const QString &module, const QString &path);

signals:
    void quitRequested(bool &allowed);
    void newDataFlow();
    void loadDataFlow();
    void saveDataFlow();
    void saveDataFlowAs();
    void executeDataFlow();
    void connectVistle();

protected:
    void closeEvent(QCloseEvent *);

private:
    Ui::MainWindow *ui;
    VistleConsole *m_console;
    Parameters *m_parameters;
    ModuleBrowser *m_moduleBrowser;

    QList<QString> loadModuleFile();
};

} //namespace gui
#endif // MAINWINDOW_H
