#ifndef UICONTROLLER_H
#define UICONTROLLER_H

#include <QObject>

#include <userinterface/vistleconnection.h>
#include "vistleobserver.h"
#include "mainwindow.h"

#include <thread>

namespace vistle {
class PythonInterface;
class PythonModule;
}

namespace gui {

class MainWindow;

class UiController : public QObject
{
   Q_OBJECT

public:
   explicit UiController(int argc, char *argv[], QObject *parent=nullptr);
   ~UiController();
   void finish();

signals:

public slots:
   void quitRequested(bool &allowed);

private slots:
   bool checkModified(const QString &reason);
   void clearDataFlowNetwork();
   void loadDataFlowNetwork();
   void saveDataFlowNetwork(const QString &filename=QString());
   void saveDataFlowNetworkAs(const QString &filename=QString());
   void executeDataFlowNetwork();
   void connectVistle();

    void moduleSelectionChanged();

    void newParameter(int moduleId, QString parameterName);
    void parameterValueChanged(int moduleId, QString parameterName);

private:
    vistle::VistleConnection *m_vistleConnection;
    vistle::UserInterface *m_ui;
    vistle::PythonInterface *m_python;
    vistle::PythonModule *m_pythonMod;
    std::thread *m_thread;
    DataFlowNetwork *m_scene;

    VistleObserver m_observer;
    MainWindow m_mainWindow;

    QString m_currentFile;
};

} // namespace gui
#endif // UICONTROLLER_H
