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
#include "parameters.h"
#include "modulebrowser.h"
#include "vistleconsole.h"


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
    m_console->append(debugMsg);
}

void MainWindow::newModule_msg(int moduleId, const boost::uuids::uuid &spawnUuid, QString moduleName)
{
    m_scene->addModule(moduleId, spawnUuid, moduleName);
#if 0
    QString text = "Module started: " + moduleName + " with ID: " + QString::number(moduleId);
    m_console->append(text);
#endif
}

void MainWindow::deleteModule_msg(int moduleId)
{
    m_scene->deleteModule(moduleId);
#if 0
    QString text = "Module deleted: " + QString::number(moduleId);
    m_console->append(text);
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
    m_console->append(text);
#endif
    if (parameterName == "_position") {
       if (Module *m = m_scene->findModule(moduleId)) {
          vistle::Parameter *p = vistle::VistleConnection::the().getParameter(moduleId, "_position");
          vistle::VectorParameter *vp = dynamic_cast<vistle::VectorParameter *>(p);
          if (vp && vp->isDefault()) {
             m->sendPosition();
          }
       }
    }
}

void MainWindow::parameterValueChanged_msg(int moduleId, QString parameterName)
{
#if 0
    QString text = "Parameter value changed on ID: " + QString::number(moduleId) + ":" + parameterName;
    m_console->append(text);
#endif
    if (parameterName == "_position") {
       if (Module *m = m_scene->findModule(moduleId)) {
          vistle::Parameter *p = vistle::VistleConnection::the().getParameter(moduleId, "_position");
          vistle::VectorParameter *vp = dynamic_cast<vistle::VectorParameter *>(p);
          if (vp && !vp->isDefault()) {
             vistle::ParamVector pos = vp->getValue();
             m->setPos(pos[0], pos[1]);
          }
       }
    }
}

void MainWindow::parameterChoicesChanged_msg(int moduleId, QString parameterName)
{
}

void MainWindow::newPort_msg(int moduleId, QString portName)
{
    QString text = "New port on ID: " + QString::number(moduleId) + ":" + portName;
    m_console->append(text);
}

void MainWindow::newConnection_msg(int fromId, QString fromName,
                                   int toId, QString toName)
{
    QString text = "New Connection: " + QString::number(fromId) + ":" + fromName + " -> " + QString::number(toId) + ":" + toName;
    m_console->append(text);
}

void MainWindow::deleteConnection_msg(int fromId, QString fromName,
                                      int toId, QString toName)
{
    QString text = "Connection removed: " + QString::number(fromId) + ":" + fromName + " -> " + QString::number(toId) + ":" + toName;
    m_console->append(text);
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
   } else {
      parameters()->setModule(0);
   }
   ui->parameterDock->setWindowTitle(title);
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

    if (Parameters *p = parameters())
       p->setVistleObserver(observer);
}

void MainWindow::setVistleConnection(vistle::VistleConnection *conn)
{
    m_scene->setRunner(conn);
    m_vistleConnection = conn;

    if (Parameters *p = parameters()) {
       p->setVistleConnection(conn);
    }
}

Parameters *MainWindow::parameters() const
{
   return m_parameters;
}

/*!
 * \brief MainWindow::on_dragButton_clicked button that mimics a dragging operation
 */
void MainWindow::on_dragButton_clicked()
{
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    mimeData->setText("Test");
    drag->setMimeData(mimeData);
    drag->start();
}

/*!
 * \brief MainWindow::on_sortButton_clicked button that sorts the modules in the scene
 */
void MainWindow::on_sortButton_clicked()
{
    m_scene->sortModules();
}

/*!
 * \brief MainWindow::on_invertModulesButton_clicked button that inverts the orientation of the modules in the scene
 */
void MainWindow::on_invertModulesButton_clicked()
{
    m_scene->invertModules();
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

#if 0
    QFile inputFile("/mnt/raid/home/hpcskimb/vistle/vistle/gui/module_list.txt");
    if (inputFile.open(QIODevice::ReadOnly)) {
       QTextStream in(&inputFile);
       while (!in.atEnd())
       {
          QString line = in.readLine();
          ui->moduleListWidget->addItem(line);
          moduleList.append(line);
       }
    } else {
        ///\todo some sort of error message/handling, perhaps just a msgbox?
    }
    inputFile.close();
#endif

    return moduleList;
}

} //namespace gui
