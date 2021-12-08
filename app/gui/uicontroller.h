#ifndef UICONTROLLER_H
#define UICONTROLLER_H

#include <QObject>

#include <vistle/userinterface/vistleconnection.h>
#include "vistleobserver.h"
#include "mainwindow.h"

#include <thread>

namespace vistle {
class PythonInterface;
class PythonModule;
} // namespace vistle

namespace gui {

class MainWindow;

class UiController: public QObject {
    Q_OBJECT

public:
    explicit UiController(int argc, char *argv[], QObject *parent = nullptr);
    ~UiController();
    void init();
    void finish();

signals:

public slots:
    void quitRequested(bool &allowed);

    void aboutVistle();
    void aboutQt();

private slots:
    void setModified(bool mod);
    bool checkModified(const QString &reason);
    void clearDataFlowNetwork();
    void loadDataFlowNetwork();
    void saveDataFlowNetwork(const QString &filename = QString());
    void saveDataFlowNetworkAs(const QString &filename = QString());
    void executeDataFlowNetwork();
    void connectVistle();

    void moduleSelectionChanged();

    void newParameter(int moduleId, QString parameterName);
    void parameterValueChanged(int moduleId, QString parameterName);

    void statusUpdated(int id, QString text, int prio);
    void setCurrentFile(QString file);
    void setSessionUrl(QString url);

    void showConnectionInfo();
    void copyConnectionInfo();
    void screenshot(QString imageefile);

private:
    vistle::VistleConnection *m_vistleConnection;
    vistle::UserInterface *m_ui;
    vistle::PythonInterface *m_python;
    vistle::PythonModule *m_pythonMod;
    std::thread *m_thread;
    DataFlowNetwork *m_scene;

    VistleObserver m_observer;
    MainWindow *m_mainWindow;

    QString m_currentFile;
    bool m_modified = false;
    std::string m_pythonDir;

    QString m_sessionUrl;
};

} // namespace gui
#endif // UICONTROLLER_H
