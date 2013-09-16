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

signals:
    void quitRequested(bool &allowed);
    void newDataFlow();
    void loadDataFlow();
    void saveDataFlow();
    void saveDataFlowAs();
    void executeDataFlow();

protected:
    void closeEvent(QCloseEvent *);

private slots:
    void moduleInfo(const QString &text);


private:
    Ui::MainWindow *ui = nullptr;
    VistleConsole *m_console = nullptr;
    Parameters *m_parameters = nullptr;
    ModuleBrowser *m_moduleBrowser = nullptr;

    QList<QString> loadModuleFile();
};

} //namespace gui
#endif // MAINWINDOW_H
