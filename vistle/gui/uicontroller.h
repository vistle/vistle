#ifndef UICONTROLLER_H
#define UICONTROLLER_H

#include <QObject>

#include <userinterface/vistleconnection.h>
#include "vistleobserver.h"
#include "mainwindow.h"

namespace boost {
class thread;
}

namespace gui {

class MainWindow;

class UiController : public QObject
{
   Q_OBJECT

public:
   explicit UiController(int argc, char *argv[], QObject *parent=nullptr);
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

    void moduleSelectionChanged();

    void newParameter(int moduleId, QString parameterName);
    void parameterValueChanged(int moduleId, QString parameterName);

private:
    vistle::VistleConnection *m_vistleConnection = nullptr;
    vistle::UserInterface *m_ui = nullptr;
    boost::thread *m_thread = nullptr;
    DataFlowNetwork *m_scene = nullptr;

    VistleObserver m_observer;
    MainWindow m_mainWindow;

    QString m_currentFile;
};

} // namespace gui
#endif // UICONTROLLER_H
