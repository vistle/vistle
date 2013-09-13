/*********************************************************************************/
/*! \file mainwindow.cpp
 *
 * - re-implementation of Qt's QMainWindow, which handles connections between the back
 * end and the front end UI components.
 * - Creates slots connected to the StatePrinter, which print debug messages to the UI.
 * - Has a drag-drop event set, which handles drops from the UI into the GraphicsView.
 *
 */
/**********************************************************************************/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "modifieddialog.h"
#include "parameters.h"
#include "modulebrowser.h"
#include "vistleconsole.h"

#include <userinterface/pythonembed.h>


#include <QString>
#include <QMessageBox>
#include <QByteArray>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QProcess>
#include <QDrag>
#include <QMimeData>
#include <QFile>
#include <QTextStream>
#include <QDropEvent>
#include <QFileDialog>
#include <QDir>

namespace gui {

/*!
 * \brief MainWindow::MainWindow
 * \param parent
 *
 * \todo add more ports, that are connected from the handler. These will send feedback to the
 *       UI if an action has been completed. Current idea is a debug window
 */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // declare list of names of modules, pass to the scene
    ui->setupUi(this);
    tabifyDockWidget(ui->modulesDock, ui->parameterDock);

    m_console = ui->consoleWidget;
    setFocusProxy(m_console);

    m_parameters = ui->parameterEditor;

    m_moduleBrowser = ui->moduleBrowser;
    auto modules = loadModuleFile();
    m_moduleBrowser->setModules(modules);

    ui->drawArea->setAttribute(Qt::WA_AlwaysShowToolTips);
    ui->drawArea->setDragMode(QGraphicsView::RubberBandDrag);

    ///\todo declare the scene pointer in the header, then de-allocate in the destructor.
    m_scene = new Scene(ui->drawArea);
    m_scene->setMainWindow(this);

    ui->drawArea->setScene(m_scene);

    connect(m_scene, SIGNAL(selectionChanged()), SLOT(moduleSelectionChanged()));
    ui->drawArea->show();


    connect(ui->actionNew, SIGNAL(triggered()), SLOT(clearDataFlowNetwork()));
    connect(ui->actionOpen, SIGNAL(triggered()), SLOT(loadDataFlowNetwork()));
    connect(ui->actionSave, SIGNAL(triggered()), SLOT(saveDataFlowNetwork()));
    connect(ui->actionSave_As, SIGNAL(triggered()), SLOT(saveDataFlowNetworkAs()));
    connect(ui->actionExecute, SIGNAL(triggered()), SLOT(executeDataFlowNetwork()));

    ui->modulesDock->show();
    ui->modulesDock->raise();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_scene;
    ///\todo Keep track of all things created with new:
    /// scene; QDrag; mimeData;
}

/************************************************************************************/
// Begin ports
/************************************************************************************/
void MainWindow::debug_msg(QString debugMsg)
{
    m_console->appendDebug(debugMsg);
}

void MainWindow::newModule_msg(int moduleId, const boost::uuids::uuid &spawnUuid, QString moduleName)
{
    m_scene->addModule(moduleId, spawnUuid, moduleName);
#if 0
    QString text = "Module started: " + moduleName + " with ID: " + QString::number(moduleId);
    m_console->appendDebug(text);
#endif
}

void MainWindow::deleteModule_msg(int moduleId)
{
    m_scene->deleteModule(moduleId);
#if 0
    QString text = "Module deleted: " + QString::number(moduleId);
    m_console->appendDebug(text);
#endif
}

void MainWindow::moduleStateChanged_msg(int moduleId, int stateBits, Module::Status modChangeType)
{
   if (Module *m = m_scene->findModule(moduleId)) {
      m->setStatus(modChangeType);
   }

#if 0
    QString text;
    if (modChangeType == INITIALIZED) text = "Module state change on ID: " + QString::number(moduleId) + " initialized";
    if (modChangeType == KILLED) text = "Module state change on ID: " + QString::number(moduleId) + " killed";
    if (modChangeType == BUSY) text = "Module state change on ID: " + QString::number(moduleId) + " busy";

    m_console->append(text);
#endif
}

void MainWindow::newParameter_msg(int moduleId, QString parameterName)
{
#if 0
    QString text = "New parameter on ID: " + QString::number(moduleId) + ":" + parameterName;
    m_console->appendDebug(text);
#endif
#if 1
    if (parameterName == "_position") {
       if (Module *m = m_scene->findModule(moduleId)) {
          vistle::Parameter *p = vistle::VistleConnection::the().getParameter(moduleId, "_position");
          vistle::VectorParameter *vp = dynamic_cast<vistle::VectorParameter *>(p);
          if (vp && vp->isDefault() && m->isPositionValid()) {
             m->sendPosition();
          }
       }
    }
#endif
}

void MainWindow::parameterValueChanged_msg(int moduleId, QString parameterName)
{
#if 0
   QString text = "Parameter value changed on ID: " + QString::number(moduleId) + ":" + parameterName;
   m_console->appendDebug(text);
#endif
   if (parameterName == "_position") {
      if (Module *m = m_scene->findModule(moduleId)) {
         vistle::Parameter *p = vistle::VistleConnection::the().getParameter(moduleId, "_position");
         vistle::VectorParameter *vp = dynamic_cast<vistle::VectorParameter *>(p);
         if (vp && !vp->isDefault()) {
            vistle::ParamVector pos = vp->getValue();
            m->setPos(pos[0], pos[1]);
            m->setPositionValid();
         }
      }
   }
}

void MainWindow::parameterChoicesChanged_msg(int moduleId, QString parameterName)
{
}

void MainWindow::newPort_msg(int moduleId, QString portName)
{
   if (Module *m = m_scene->findModule(moduleId)) {
      vistle::Port *port = m_vistleConnection->ui().state().portTracker()->getPort(moduleId, portName.toStdString());
      if (port) {
         m->addPort(port);
      }
   }
#if 0
    QString text = "New port on ID: " + QString::number(moduleId) + ":" + portName;
    m_console->appendDebug(text);
#endif
}

void MainWindow::newConnection_msg(int fromId, QString fromName,
                                   int toId, QString toName) {

#if 0
   QString text = "New Connection: " + QString::number(fromId) + ":" + fromName + " -> " + QString::number(toId) + ":" + toName;
   m_console->appendDebug(text);
#endif

   vistle::Port *portFrom = m_vistleConnection->ui().state().portTracker()->getPort(fromId, fromName.toStdString());
   vistle::Port *portTo = m_vistleConnection->ui().state().portTracker()->getPort(toId, toName.toStdString());

   Module *mFrom = m_scene->findModule(fromId);
   Module *mTo = m_scene->findModule(toId);

   if (mFrom && portFrom && mTo && portTo) {
      m_scene->addConnection(mFrom->getGuiPort(portFrom), mTo->getGuiPort(portTo));
   }
}

void MainWindow::deleteConnection_msg(int fromId, QString fromName,
                                      int toId, QString toName)
{
#if 0
   QString text = "Connection removed: " + QString::number(fromId) + ":" + fromName + " -> " + QString::number(toId) + ":" + toName;
   m_console->appendDebug(text);
#endif

   vistle::Port *portFrom = m_vistleConnection->ui().state().portTracker()->getPort(fromId, fromName.toStdString());
   vistle::Port *portTo = m_vistleConnection->ui().state().portTracker()->getPort(toId, toName.toStdString());

   Module *mFrom = m_scene->findModule(fromId);
   Module *mTo = m_scene->findModule(toId);

   if (mFrom && portFrom && mTo && portTo) {
      m_scene->removeConnection(mFrom->getGuiPort(portFrom), mTo->getGuiPort(portTo));
   }
}

void MainWindow::setModified(bool state)
{
   setWindowModified(state);
}

void MainWindow::moduleInfo(const QString &text)
{
   m_console->appendInfo(text);
}

void MainWindow::moduleSelectionChanged()
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
      parameters()->setModule(m->id());
      title = QString("Parameters: %1").arg(m->name());
      parameters()->show();
      ui->parameterDock->show();
      ui->parameterDock->raise();
   } else {
      parameters()->setModule(0);
      ui->modulesDock->show();
      ui->modulesDock->raise();
   }
   ui->parameterDock->setWindowTitle(title);
}

bool MainWindow::checkModified(const QString &reason)
{
   //std::cerr << "modification count: " << m_observer->modificationCount() << std::endl;
   if (m_observer->modificationCount() == 0)
      return true;

   ModifiedDialog d(reason, this);

   int res = d.exec();
   std::cerr << "res: " << res << std::endl;
   if (res == QMessageBox::Save) {
      saveDataFlowNetwork();
      return true;
   }
   return res == QMessageBox::Discard;
}

void MainWindow::setFilename(const QString &filename)
{
   m_currentFile = filename;
   setWindowFilePath(filename);
   if (m_currentFile.isEmpty()) {
      setWindowTitle("Vistle");
   } else {
      setWindowTitle(QString("Vistle - %1").arg(m_currentFile));
   }
}

void MainWindow::clearDataFlowNetwork()
{
   if (!checkModified("New"))
      return;

   m_vistleConnection->resetDataFlowNetwork();
   setFilename(QString());
   m_observer->resetModificationCount();
}

void MainWindow::loadDataFlowNetwork()
{
   if (!checkModified("Open"))
      return;

   QString dir = m_currentFile.isEmpty() ? QDir::currentPath() : m_currentFile;
   QString filename = QFileDialog::getOpenFileName(this,
      tr("Open Data Flow Network"),
      dir,
      tr("Vistle files (*.vsl);;Python files (*.py);;All files (*)"));

   if (filename.isEmpty())
      return;

   clearDataFlowNetwork();

   setFilename(filename);

   vistle::PythonInterface::the().exec_file(filename.toStdString());

   m_observer->resetModificationCount();
}

void MainWindow::saveDataFlowNetwork(const QString &filename)
{
   if(filename.isEmpty()) {
      if (!m_currentFile.isEmpty())
         saveDataFlowNetwork(m_currentFile);
      else
         saveDataFlowNetworkAs();
   } else {
      std::cerr << "writing to " << filename.toStdString() << std::endl;
      setFilename(filename);
      std::string cmd = "save(\"";
      cmd += filename.toStdString();
      cmd += "\")";
      vistle::PythonInterface::the().exec(cmd);
      m_observer->resetModificationCount();
   }
}

void MainWindow::saveDataFlowNetworkAs(const QString &filename)
{
   QString newFile = QFileDialog::getSaveFileName(this,
      tr("Save Data Flow Network"),
      filename.isEmpty() ? QDir::currentPath() : filename,
      tr("Vistle files (*.vsl);;Python files (*.py);;All files (*)"));

   if (!newFile.isEmpty())
      saveDataFlowNetwork(newFile);
}

void MainWindow::executeDataFlowNetwork()
{
   m_vistleConnection->executeSources();
}

/************************************************************************************/
// End ports
/************************************************************************************/

void MainWindow::setVistleobserver(VistleObserver *observer)
{
    m_observer = observer;
    
    // connect the printer signals to the appropriate ports
    ///\todo should these connections be pushed to another method?
    connect(m_observer, SIGNAL(debug_s(QString)),
            this, SLOT(debug_msg(QString)));
    connect(m_observer, SIGNAL(newModule_s(int, boost::uuids::uuid, QString)),
            this, SLOT(newModule_msg(int, boost::uuids::uuid, QString)));
    connect(m_observer, SIGNAL(deleteModule_s(int)),
            this, SLOT(deleteModule_msg(int)));
    connect(m_observer, SIGNAL(moduleStateChanged_s(int, int, Module::Status)),
            this, SLOT(moduleStateChanged_msg(int, int, Module::Status)));
    connect(m_observer, SIGNAL(newParameter_s(int, QString)),
            this, SLOT(newParameter_msg(int, QString)));
    connect(m_observer, SIGNAL(parameterValueChanged_s(int, QString)),
            this, SLOT(parameterValueChanged_msg(int, QString)));
    connect(m_observer, SIGNAL(newPort_s(int, QString)),
            this, SLOT(newPort_msg(int, QString)));
    connect(m_observer, SIGNAL(newConnection_s(int, QString, int, QString)),
            this, SLOT(newConnection_msg(int, QString, int, QString)));
    connect(m_observer, SIGNAL(deleteConnection_s(int, QString, int, QString)),
            this, SLOT(deleteConnection_msg(int, QString, int, QString)));

    connect(m_observer, SIGNAL(modified(bool)), this, SLOT(setModified(bool)));
    connect(m_observer, SIGNAL(info_s(QString)), this, SLOT(moduleInfo(const QString&)));

    if (Parameters *p = parameters())
       p->setVistleObserver(observer);
}

void MainWindow::setVistleConnection(vistle::VistleConnection *conn)
{
    m_scene->setVistleConnection(conn);
    m_vistleConnection = conn;

    if (Parameters *p = parameters()) {
       p->setVistleConnection(conn);
    }
}

Parameters *MainWindow::parameters() const
{
   return m_parameters;
}

DataFlowView *MainWindow::dataFlowView() const
{
   return ui->drawArea;
}

/*!
 * \brief MainWindow::loadModuleFile read the list of modules.
 *
 * The module list is read in and put inside a list widget
 */
QList<QString> MainWindow::loadModuleFile()
{
    QList<QString> moduleList;

    moduleList << "Add";
    moduleList << "Collect";
    moduleList << "CuttingSurface";
    moduleList << "Gendat";
    moduleList << "ReadCovise";
    moduleList << "Replicate";
    moduleList << "Color";
    moduleList << "Extrema";
    moduleList << "IsoSurface";
    moduleList << "ReadFOAM";
    moduleList << "ShowUSG";
    moduleList << "CellToVert";
    moduleList << "CutGeometry";
    moduleList << "Gather";
    moduleList << "OSGRenderer";
    moduleList << "ReadVistle";
    moduleList << "WriteVistle";


    return moduleList;
}

} //namespace gui
