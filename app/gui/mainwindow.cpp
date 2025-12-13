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
#include "moduleview.h"
#include "vistleconsole.h"

#include <QString>
#include <QSettings>

namespace gui {

/*!
 * \brief MainWindow::MainWindow
 * \param parent
 *
 * \todo add more ports, that are connected from the handler. These will send feedback to the
 *       UI if an action has been completed. Current idea is a debug window
 */
MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent), ui(new Ui::MainWindow), m_console(nullptr), m_parameters(nullptr), m_moduleBrowser(nullptr)
{
    // declare list of names of modules, pass to the scene
    ui->setupUi(this);

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    tabifyDockWidget(ui->modulesDock, ui->moduleViewDock);

    m_console = ui->consoleWidget;

    //m_parameters = ui->parameterEditor;
    m_parameters = new Parameters();
    m_parameters->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
    m_parameters->adjustSize();
    m_parameters->show();
    ui->moduleView->parameterScroller()->setWidgetResizable(true);
    ui->moduleView->parameterScroller()->setWidget(m_parameters);
    ui->moduleView->parameterScroller()->show();

    m_moduleBrowser = ui->moduleBrowser;

    ui->drawArea->show();

    connect(ui->actionQuit, SIGNAL(triggered()), SLOT(close()));
    ui->actionQuit->setShortcut(QKeySequence::StandardKey::Quit);
    connect(ui->actionNew, SIGNAL(triggered()), SIGNAL(newDataFlow()));
    ui->actionNew->setShortcut(QKeySequence::StandardKey::New);
    connect(ui->actionOpen, &QAction::triggered, [this]() { emit loadDataFlowOnHub(); });
    ui->actionOpen->setShortcut(QKeySequence::StandardKey::Open);
    connect(ui->actionOpenOnGui, SIGNAL(triggered()), SIGNAL(loadDataFlowOnGui()));
    connect(ui->actionOpenOnHub, SIGNAL(triggered()), SIGNAL(loadDataFlowOnHub()));
    connect(ui->actionSave, SIGNAL(triggered()), SIGNAL(saveDataFlow()));
    ui->actionSave->setShortcut(QKeySequence::StandardKey::Save);
    connect(ui->actionSaveOnGui, SIGNAL(triggered()), SIGNAL(saveDataFlowOnGui()));
    connect(ui->actionSaveOnHub, SIGNAL(triggered()), SIGNAL(saveDataFlowOnHub()));
    ui->actionSaveOnHub->setShortcut(QKeySequence::StandardKey::SaveAs);
    connect(ui->actionExecute, SIGNAL(triggered()), SIGNAL(executeDataFlow()));
    ui->actionExecute->setShortcut(QKeySequence::StandardKey::Refresh);
    connect(ui->actionConnect, SIGNAL(triggered()), SIGNAL(connectVistle()));
    connect(ui->actionShow_Session_URL, SIGNAL(triggered()), SIGNAL(showSessionUrl()));
    connect(ui->actionCopy_Session_URL, SIGNAL(triggered()), SIGNAL(copySessionUrl()));
    ui->actionSelect_All->setShortcut(QKeySequence::StandardKey::SelectAll);
    connect(ui->actionSelect_All, SIGNAL(triggered()), SIGNAL(selectAllModules()));
    connect(ui->actionSelectInvert, SIGNAL(triggered()), SIGNAL(selectInvert()));
    QList<QKeySequence> deleteKeys;
    deleteKeys.push_back(QKeySequence::StandardKey::Delete);
#ifdef Q_OS_MAC
    deleteKeys.push_back(Qt::Key_Backspace);
#endif
    ui->actionDelete->setShortcuts(deleteKeys);
    connect(ui->actionDelete, SIGNAL(triggered()), SIGNAL(deleteSelectedModules()));
    connect(ui->actionSelectClear, SIGNAL(triggered()), SIGNAL(selectClear()));
    connect(ui->actionSnapToGrid, SIGNAL(triggered(bool)), SIGNAL(snapToGridChanged(bool)));
    connect(ui->actionViewAll, SIGNAL(triggered()), SIGNAL(zoomAll()));
    connect(ui->actionViewOrig, SIGNAL(triggered()), SIGNAL(zoomOrig()));
    connect(ui->actionSelectSinks, SIGNAL(triggered()), SIGNAL(selectSinkModules()));
    connect(ui->actionSelectSources, SIGNAL(triggered()), SIGNAL(selectSourceModules()));

    connect(ui->actionNative_Menubar, &QAction::toggled, [this](bool native) {
        if (menuBar())
            menuBar()->setNativeMenuBar(native);
    });

    connect(ui->actionAbout, SIGNAL(triggered()), SIGNAL(aboutVistle()));
    ui->actionAbout->setMenuRole(QAction::AboutRole);
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), SIGNAL(aboutQt()));
    ui->actionAbout_Qt->setMenuRole(QAction::AboutQtRole);

    //setFocusProxy(ui->modulesDock);
    ui->modulesDock->setFocusProxy(ui->moduleBrowser);
    ui->modulesDock->show();
    ui->modulesDock->raise();
    ui->modulesDock->setFocus();

#ifndef HAVE_PYTHON
    ui->actionSaveOnGui->setEnabled(false);
    ui->actionSaveOnGui->setVisible(false);
    ui->actionOpenOnGui->setEnabled(false);
    ui->actionOpenOnGui->setVisible(false);
#endif

    readSettings();

    if (menuBar())
        ui->actionNative_Menubar->setChecked(menuBar()->isNativeMenuBar());
}

MainWindow::~MainWindow()
{
    delete ui;
    ///\todo Keep track of all things created with new:
    /// scene; QDrag; mimeData;
}

void MainWindow::setInteractionEnabled(bool enable)
{
#ifdef HAVE_PYTHON
    ui->actionSaveOnGui->setEnabled(enable);
    ui->actionOpenOnGui->setEnabled(enable);
#endif
    ui->actionNew->setEnabled(enable);
    ui->actionQuit->setEnabled(enable);
    ui->actionOpen->setEnabled(enable);
    ui->actionOpenOnHub->setEnabled(enable);
    ui->actionSave->setEnabled(enable);
    ui->actionSaveOnHub->setEnabled(enable);
}

QToolBar *MainWindow::toolBar() const
{
    return ui->toolBar;
}

QAction *MainWindow::layerWidgetPosition() const
{
    return ui->actionArrange;
}

QMenu *MainWindow::createPopupMenu()
{
    auto menu = QMainWindow::createPopupMenu();
    menu->addAction(ui->actionNative_Menubar);
    return menu;
}

void MainWindow::setQuitOnExit(bool qoe)
{
    if (ui->actionQuit) {
        ui->actionQuit->setMenuRole(QAction::QuitRole);
        if (qoe) {
            ui->actionQuit->setText("Quit");
            ui->actionQuit->setStatusTip("Quit Vistle session");
        } else {
            ui->actionQuit->setText("Leave");
            ui->actionQuit->setStatusTip("Quit Vistle GUI");
        }
    }
}

void MainWindow::setModified(bool state)
{
    setWindowModified(state);
}

void MainWindow::newHub(int hub, const QString &hubName, int nranks, const QString &address, int port,
                        const QString &logname, const QString &realname, bool hasUi, const QString &systype,
                        const QString &arch, const QString &info, const QString &version)
{
    m_moduleBrowser->addHub(hub, hubName, nranks, address, port, logname, realname, hasUi, systype, arch, info,
                            version);
}

void MainWindow::deleteHub(int hub)
{
    m_moduleBrowser->removeHub(hub);
}

void MainWindow::moduleAvailable(int hub, const QString &mod, const QString &path, const QString &category,
                                 const QString &description)
{
    m_moduleBrowser->addModule(hub, mod, path, category, description);
}

void MainWindow::enableConnectButton(bool state)
{
    ui->actionConnect->setEnabled(state);
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

QDockWidget *MainWindow::moduleViewDock() const
{
    return ui->moduleViewDock;
}

QDockWidget *MainWindow::modulesDock() const
{
    return ui->modulesDock;
}

Parameters *MainWindow::parameters() const
{
    return m_parameters;
}

ModuleView *MainWindow::moduleView() const
{
    return ui->moduleView;
}

DataFlowView *MainWindow::dataFlowView() const
{
    return ui->drawArea;
}

VistleConsole *MainWindow::console() const
{
    return m_console;
}

ModuleBrowser *MainWindow::moduleBrowser() const
{
    return m_moduleBrowser;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    writeSettings();

    bool allowed = true;
    emit quitRequested(allowed);
    if (!allowed) {
        e->ignore();
        return;
    }

    QWidgetList allToplevelWidgets = QApplication::topLevelWidgets();
    for (auto w: allToplevelWidgets) {
        w->close();
    }
}

void MainWindow::readSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    auto geo = settings.value("geometry", size()).toByteArray();
    restoreGeometry(geo);
    // calling this twice somehow enables restoring the fullscreen window on a second monitor, restoring the non-maximized window on a second monitor still fails
    restoreGeometry(geo);
    restoreState(settings.value("windowState").toByteArray());

    if (menuBar())
        menuBar()->setNativeMenuBar(settings.value("nativeMenuBar", true).toBool());

    bool snapToGrid = settings.value("snapToGrid", ui->actionSnapToGrid->isChecked()).toBool();
    ui->actionSnapToGrid->setChecked(snapToGrid);
    emit snapToGridChanged(snapToGrid);

    settings.endGroup();
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");

    settings.setValue("geometry", saveGeometry());

    settings.setValue("windowState", saveState());

    if (menuBar())
        settings.setValue("nativeMenuBar", menuBar()->isNativeMenuBar());

    settings.setValue("snapToGrid", ui->actionSnapToGrid->isChecked());

    settings.endGroup();
}

void MainWindow::enableSnapToGrid(bool snap)
{
    ui->actionSnapToGrid->setChecked(snap);
}

bool MainWindow::isSnapToGrid() const
{
    return ui->actionSnapToGrid->isChecked();
}

} //namespace gui
