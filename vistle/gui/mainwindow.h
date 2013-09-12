#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QList>
#include <QMainWindow>

#include <userinterface/userinterface.h>
#include <userinterface/vistleconnection.h>

#include "vistleconsole.h"
#include "scene.h"
#include "vistleobserver.h"

namespace gui {

class Parameters;
class DataFlowView;
class ModuleBrowser;

namespace Ui {
class MainWindow;

} //namespace Ui

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void printDebug(QString msg);
    void setVistleobserver(VistleObserver *printer);
    void setVistleConnection(vistle::VistleConnection *conn);

    Parameters *parameters() const;
    DataFlowView *dataFlowView() const;

private slots:
    void on_dragButton_clicked();
    void on_sortButton_clicked();
    void on_invertModulesButton_clicked();

    void debug_msg(QString debugMsg);
    void newModule_msg(int moduleId, const boost::uuids::uuid &spawnUuid, QString moduleName);
    void deleteModule_msg(int moduleId);
    void moduleStateChanged_msg(int moduleId, int stateBits, Module::Status modChangeType);
    void newParameter_msg(int moduleId, QString parameterName);
    void parameterValueChanged_msg(int moduleId, QString parameterName);
    void parameterChoicesChanged_msg(int moduleId, QString parameterName);
    void newPort_msg(int moduleId, QString portName);
    void newConnection_msg(int fromId, QString fromName,
                         int toId, QString toName);
    void deleteConnection_msg(int fromId, QString fromName,
                            int toId, QString toName);

    void setModified(bool state);
    void moduleInfo(const QString &text);

    void moduleSelectionChanged();
    void clearDataFlowNetwork();
    void loadDataFlowNetwork();
    void saveDataFlowNetwork(const QString &filename = QString());
    void saveDataFlowNetworkAs(const QString &filename = QString());
    void executeDataFlowNetwork();

private:
    Ui::MainWindow *ui = nullptr;
    VistleConsole *m_console = nullptr;
    Parameters *m_parameters = nullptr;
    ModuleBrowser *m_moduleBrowser = nullptr;

    vistle::VistleConnection *m_vistleConnection = nullptr;
    QList<QString> loadModuleFile();
    void addModule(QString modName, QPointF dropPos);
    Scene *m_scene = nullptr;
    VistleObserver *m_observer = nullptr;

    QString m_currentFile;

    bool checkModified(const QString &reason);
    void setFilename(const QString &filename);
};

} //namespace gui
#endif // MAINWINDOW_H
