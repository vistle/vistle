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

    //m_parameters = ui->parameterEditor;
    m_parameters = new Parameters();
    m_parameters->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
    m_parameters->adjustSize();
    m_parameters->show();
    ui->parameterScroller->setWidgetResizable(true);
    ui->parameterScroller->setWidget(m_parameters);
    ui->parameterScroller->show();

    m_moduleBrowser = ui->moduleBrowser;

    ui->drawArea->setAttribute(Qt::WA_AlwaysShowToolTips);
    ui->drawArea->setDragMode(QGraphicsView::RubberBandDrag);
    ui->drawArea->show();

    connect(ui->actionQuit, SIGNAL(triggered()), SLOT(close()));
    connect(ui->actionNew, SIGNAL(triggered()), SIGNAL(newDataFlow()));
    connect(ui->actionOpen, SIGNAL(triggered()), SIGNAL(loadDataFlow()));
    connect(ui->actionSave, SIGNAL(triggered()), SIGNAL(saveDataFlow()));
    connect(ui->actionSave_As, SIGNAL(triggered()), SIGNAL(saveDataFlowAs()));
    connect(ui->actionExecute, SIGNAL(triggered()), SIGNAL(executeDataFlow()));
    connect(ui->actionConnect, SIGNAL(triggered()), SIGNAL(connectVistle()));

    ui->modulesDock->show();
    ui->modulesDock->raise();
}

MainWindow::~MainWindow()
{
    delete ui;
    ///\todo Keep track of all things created with new:
    /// scene; QDrag; mimeData;
}

void MainWindow::setQuitOnExit(bool qoe)
{
   if (ui->actionQuit) {
      if (qoe) {
         ui->actionQuit->setText("Quit");
         ui->actionQuit->setToolTip("Quit Vistle Session");
      } else {
         ui->actionQuit->setText("Leave");
         ui->actionQuit->setToolTip("Quit Vistle GUI");
      }
   }
}

void MainWindow::setModified(bool state)
{
   setWindowModified(state);
}

void MainWindow::moduleAvailable(int hub, const QString &hubName, const QString &mod, const QString &path) {

    m_moduleBrowser->addModule(hub, hubName, mod, path);
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

} //namespace gui
