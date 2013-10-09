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

namespace gui {

/*!
 * \brief MainWindow::MainWindow
 * \param parent
 *
 * \todo add more ports, that are connected from the handler. These will send feedback to the
 *       UI if an action has been completed. Current idea is a debug window
 */
MainWindow::MainWindow(QWidget *parent)
   : QMainWindow(parent)
   , ui(new Ui::MainWindow)
   , m_console(nullptr)
   , m_parameters(nullptr)
   , m_moduleBrowser(nullptr)
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
    ui->drawArea->show();

    connect(ui->actionQuit, SIGNAL(triggered()), SLOT(close()));
    connect(ui->actionNew, SIGNAL(triggered()), SIGNAL(newDataFlow()));
    connect(ui->actionOpen, SIGNAL(triggered()), SIGNAL(loadDataFlow()));
    connect(ui->actionSave, SIGNAL(triggered()), SIGNAL(saveDataFlow()));
    connect(ui->actionSave_As, SIGNAL(triggered()), SIGNAL(saveDataFlowAs()));
    connect(ui->actionExecute, SIGNAL(triggered()), SIGNAL(executeDataFlow()));

    ui->modulesDock->show();
    ui->modulesDock->raise();
}

MainWindow::~MainWindow()
{
    delete ui;
    ///\todo Keep track of all things created with new:
    /// scene; QDrag; mimeData;
}

void MainWindow::setModified(bool state)
{
   setWindowModified(state);
}

void MainWindow::moduleInfo(const QString &text)
{
   m_console->appendInfo(text);
}

void MainWindow::setFilename(const QString &filename)
{
   setWindowFilePath(filename);
   if (filename.isEmpty()) {
      setWindowTitle("Vistle [*]");
   } else {
      setWindowTitle(QString("Vistle - %1 [*]").arg(filename));
   }
}

QDockWidget *MainWindow::consoleDock() const
{
   return ui->consoleDock;
}

QDockWidget *MainWindow::parameterDock() const
{
   return ui->parameterDock;
}

QDockWidget *MainWindow::modulesDock() const
{
   return ui->modulesDock;
}

Parameters *MainWindow::parameters() const
{
   return m_parameters;
}

DataFlowView *MainWindow::dataFlowView() const
{
   return ui->drawArea;
}

VistleConsole *MainWindow::console() const
{
   return m_console;
}

void MainWindow::closeEvent(QCloseEvent *e) {

   bool allowed = true;
   emit quitRequested(allowed);
   if (!allowed) {
      e->ignore();
   }
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
    moduleList << "COVER";
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
