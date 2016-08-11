#include "uicontroller.h"
#include "modifieddialog.h"
#include "dataflownetwork.h"
#include "dataflowview.h"
#include "parameters.h"
#include "vistleconsole.h"
#include <userinterface/pythoninterface.h>
#include <userinterface/pythonmodule.h>
#include <util/findself.h>

#include <boost/thread.hpp>

#include <QDir>
#include <QFileDialog>
#include <QDockWidget>
#include <QGuiApplication>

namespace gui {

UiController::UiController(int argc, char *argv[], QObject *parent)
: QObject(parent)
, m_vistleConnection(nullptr)
, m_ui(nullptr)
, m_python(nullptr)
, m_pythonMod(nullptr)
, m_thread(nullptr)
, m_scene(nullptr)
{
   std::string host = "localhost";
   unsigned short port = 31093;

   bool quitOnExit = false;
   if (argc > 1 && argv[1] == std::string("-from-vistle")) {
      quitOnExit = true;
      argv[1] = argv[0];
      --argc;
      ++argv;
   }

   if (argc > 1) {
      host = argv[1];
   }
   if (argc > 2) {
      port = atoi(argv[2]);
   }

   m_python = new vistle::PythonInterface("Vistle GUI");

   m_mainWindow.parameters()->setVistleObserver(&m_observer);
   m_mainWindow.setQuitOnExit(quitOnExit);
   m_ui = new vistle::UserInterface(host, port, &m_observer);
   if (!m_ui->isConnected()) {
      std::cerr << "UI: not yet connected to " << host << ":" << port << std::endl;
   } else {
       m_mainWindow.enableConnectButton(false);
   }
   m_ui->registerObserver(&m_observer);
   m_vistleConnection = new vistle::VistleConnection(*m_ui);
   m_vistleConnection->setQuitOnExit(quitOnExit);
   m_pythonMod = new vistle::PythonModule(m_vistleConnection, vistle::getbindir(argc, argv) + "/../share/vistle/");
   m_thread = new boost::thread(boost::ref(*m_vistleConnection));
   m_mainWindow.parameters()->setVistleConnection(m_vistleConnection);

    ///\todo declare the scene pointer in the header, then de-allocate in the destructor.
   m_scene = new DataFlowNetwork(m_vistleConnection, m_mainWindow.dataFlowView());
   m_mainWindow.dataFlowView()->setScene(m_scene);

   connect(&m_mainWindow, SIGNAL(quitRequested(bool &)), SLOT(quitRequested(bool &)));
   connect(&m_mainWindow, SIGNAL(newDataFlow()), SLOT(clearDataFlowNetwork()));
   connect(&m_mainWindow, SIGNAL(loadDataFlow()), SLOT(loadDataFlowNetwork()));
   connect(&m_mainWindow, SIGNAL(saveDataFlow()), SLOT(saveDataFlowNetwork()));
   connect(&m_mainWindow, SIGNAL(saveDataFlowAs()), SLOT(saveDataFlowNetworkAs()));
   connect(&m_mainWindow, SIGNAL(executeDataFlow()), SLOT(executeDataFlowNetwork()));
   connect(&m_mainWindow, SIGNAL(connectVistle()), SLOT(connectVistle()));

   connect(m_scene, SIGNAL(selectionChanged()), SLOT(moduleSelectionChanged()));

   connect(&m_observer, SIGNAL(quit_s()), qApp, SLOT(quit()));
   connect(&m_observer, SIGNAL(info_s(QString, int)),
      m_mainWindow.console(), SLOT(appendInfo(QString, int)));
   connect(&m_observer, SIGNAL(modified(bool)),
      &m_mainWindow, SLOT(setModified(bool)));
   connect(&m_observer, SIGNAL(newModule_s(int, boost::uuids::uuid, QString)),
      m_scene, SLOT(addModule(int,boost::uuids::uuid,QString)));
   connect(&m_observer, SIGNAL(deleteModule_s(int)),
      m_scene, SLOT(deleteModule(int)));
   connect(&m_observer, SIGNAL(moduleStateChanged_s(int, int)),
      m_scene, SLOT(moduleStateChanged(int, int)));
   connect(&m_observer, SIGNAL(newPort_s(int, QString)),
      m_scene, SLOT(newPort(int, QString)));
   connect(&m_observer, SIGNAL(deletePort_s(int, QString)),
      m_scene, SLOT(deletePort(int, QString)));
   connect(&m_observer, SIGNAL(newConnection_s(int, QString, int, QString)),
      m_scene, SLOT(newConnection(int, QString, int, QString)));
   connect(&m_observer, SIGNAL(deleteConnection_s(int, QString, int, QString)),
      m_scene, SLOT(deleteConnection(int, QString, int, QString)));

   connect(&m_observer, SIGNAL(newParameter_s(int, QString)),
           SLOT(newParameter(int, QString)));
   connect(&m_observer, SIGNAL(parameterValueChanged_s(int, QString)),
           SLOT(parameterValueChanged(int, QString)));

   connect(&m_observer, SIGNAL(moduleAvailable_s(int, QString, QString, QString)),
           &m_mainWindow, SLOT(moduleAvailable(int, QString, QString, QString)));

   m_mainWindow.show();
}

UiController::~UiController()
{
}

void UiController::finish() {

   delete m_scene;
   m_scene = nullptr;

   m_vistleConnection->cancel();
   m_thread->join();
   delete m_thread;
   m_thread = nullptr;

   delete m_pythonMod;
   m_pythonMod = nullptr;

   delete m_ui;
   m_ui = nullptr;

   delete m_vistleConnection;
   m_vistleConnection = nullptr;

   delete m_python;
   m_python = nullptr;
}

void UiController::quitRequested(bool &allowed) {

   allowed = checkModified("Quit");
}

bool UiController::checkModified(const QString &reason)
{
   //std::cerr << "modification count: " << m_observer->modificationCount() << std::endl;
   if (m_observer.modificationCount() == 0)
      return true;

   ModifiedDialog d(reason, &m_mainWindow);

   int res = d.exec();
   //std::cerr << "res: " << res << std::endl;
   if (res == QMessageBox::Save) {
      saveDataFlowNetwork();
      return true;
   }
   return res == QMessageBox::Discard;
}

void UiController::clearDataFlowNetwork()
{
   if (!checkModified("New"))
      return;

   m_vistleConnection->resetDataFlowNetwork();
   m_mainWindow.setFilename(QString());
   m_observer.resetModificationCount();
}

void UiController::loadDataFlowNetwork()
{
   if (!checkModified("Open"))
      return;

   QString dir = m_currentFile.isEmpty() ? QDir::currentPath() : m_currentFile;
   QString filename = QFileDialog::getOpenFileName(&m_mainWindow,
      tr("Open Data Flow Network"),
      dir,
      tr("Vistle files (*.vsl);;Python files (*.py);;All files (*)"));

   if (filename.isEmpty())
      return;

   clearDataFlowNetwork();

   m_mainWindow.setFilename(filename);

   vistle::PythonInterface::the().exec_file(filename.toStdString());

   m_observer.resetModificationCount();
}

void UiController::saveDataFlowNetwork(const QString &filename)
{
   if(filename.isEmpty()) {
      if (!m_currentFile.isEmpty())
         saveDataFlowNetwork(m_currentFile);
      else
         saveDataFlowNetworkAs();
   } else {
      std::cerr << "writing to " << filename.toStdString() << std::endl;
      m_mainWindow.setFilename(filename);
      std::string cmd = "save(\"";
      cmd += filename.toStdString();
      cmd += "\")";
      vistle::PythonInterface::the().exec(cmd);
      m_observer.resetModificationCount();
   }
}

void UiController::saveDataFlowNetworkAs(const QString &filename)
{
   QString newFile = QFileDialog::getSaveFileName(&m_mainWindow,
      tr("Save Data Flow Network"),
      filename.isEmpty() ? QDir::currentPath() : filename,
      tr("Vistle files (*.vsl);;Python files (*.py);;All files (*)"));

   if (!newFile.isEmpty())
      saveDataFlowNetwork(newFile);
}

void UiController::executeDataFlowNetwork()
{
   m_vistleConnection->executeSources();
}

void UiController::connectVistle()
{
   if (!m_vistleConnection->ui().isConnected())
      m_vistleConnection->ui().tryConnect();
}

void UiController::moduleSelectionChanged()
{
   auto selected = m_scene->selectedItems();
   QList<Module *> selectedModules;
   for (auto &item: selected) {
      if (Module *m = dynamic_cast<Module *>(item)) {
         selectedModules.push_back(m);
      }
   }

   QString title = "Module Parameters";
   if (selectedModules.size() == 1) {
      const Module *m = selectedModules[0];
      m_mainWindow.parameters()->setModule(m->id());
      title = QString("Parameters: %1").arg(m->name());
      m_mainWindow.parameters()->adjustSize();
      m_mainWindow.parameters()->show();
      m_mainWindow.parameterDock()->show();
      m_mainWindow.parameterDock()->raise();
   } else {
      m_mainWindow.parameters()->setModule(0);
      m_mainWindow.modulesDock()->show();
      m_mainWindow.modulesDock()->raise();
   }
   m_mainWindow.parameterDock()->setWindowTitle(title);
}

void UiController::newParameter(int moduleId, QString parameterName)
{
#if 0
    QString text = "New parameter on ID: " + QString::number(moduleId) + ":" + parameterName;
    m_console->appendDebug(text);
#endif
#if 1
    if (parameterName == "_position") {
       if (Module *m = m_scene->findModule(moduleId)) {
          auto p = vistle::VistleConnection::the().getParameter(moduleId, "_position");
          auto vp = boost::dynamic_pointer_cast<vistle::VectorParameter>(p);
          if (vp && vp->isDefault() && m->isPositionValid()) {
             m->sendPosition();
          }
       }
    }
#endif
}

void UiController::parameterValueChanged(int moduleId, QString parameterName)
{
#if 0
   QString text = "Parameter value changed on ID: " + QString::number(moduleId) + ":" + parameterName;
   m_console->appendDebug(text);
#endif
   if (parameterName == "_position") {
      if (Module *m = m_scene->findModule(moduleId)) {
         auto p = vistle::VistleConnection::the().getParameter(moduleId, "_position");
         auto vp = boost::dynamic_pointer_cast<vistle::VectorParameter>(p);
         if (vp && !vp->isDefault()) {
            vistle::ParamVector pos = vp->getValue();
            m->setPos(pos[0], pos[1]);
            m->setPositionValid();
         }
      }
   }
}

} // namespace gui
