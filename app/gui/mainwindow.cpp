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

    tabifyDockWidget(ui->modulesDock, ui->parameterDock);

    m_console = ui->consoleWidget;

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
    ui->actionQuit->setShortcut(QKeySequence::StandardKey::Quit);
    connect(ui->actionNew, SIGNAL(triggered()), SIGNAL(newDataFlow()));
    ui->actionNew->setShortcut(QKeySequence::StandardKey::New);
    connect(ui->actionOpen, SIGNAL(triggered()), SIGNAL(loadDataFlow()));
    ui->actionOpen->setShortcut(QKeySequence::StandardKey::Open);
    connect(ui->actionSave, SIGNAL(triggered()), SIGNAL(saveDataFlow()));
    ui->actionSave->setShortcut(QKeySequence::StandardKey::Save);
    connect(ui->actionSave_As, SIGNAL(triggered()), SIGNAL(saveDataFlowAs()));
    ui->actionSave_As->setShortcut(QKeySequence::StandardKey::SaveAs);
    connect(ui->actionExecute, SIGNAL(triggered()), SIGNAL(executeDataFlow()));
    ui->actionExecute->setShortcut(QKeySequence::StandardKey::Refresh);
    connect(ui->actionConnect, SIGNAL(triggered()), SIGNAL(connectVistle()));
    connect(ui->actionShow_Session_URL, SIGNAL(triggered()), SIGNAL(showSessionUrl()));
    connect(ui->actionCopy_Session_URL, SIGNAL(triggered()), SIGNAL(copySessionUrl()));
    ui->actionSelect_All->setShortcut(QKeySequence::StandardKey::SelectAll);
    connect(ui->actionSelect_All, SIGNAL(triggered()), SIGNAL(selectAllModules()));
    QList<QKeySequence> deleteKeys;
    deleteKeys.push_back(QKeySequence::StandardKey::Delete);
#ifdef Q_OS_MAC
    deleteKeys.push_back(Qt::Key_Backspace);
#endif
    ui->actionDelete->setShortcuts(deleteKeys);
    connect(ui->actionDelete, SIGNAL(triggered()), SIGNAL(deleteSelectedModules()));

    connect(ui->actionNative_Menubar, &QAction::toggled, [this](bool native) {
        if (menuBar())
            menuBar()->setNativeMenuBar(native);
    });

    connect(ui->actionAbout, SIGNAL(triggered()), SIGNAL(aboutVistle()));
    ui->actionAbout->setMenuRole(QAction::AboutRole);
    connect(ui->actionAbout_License, SIGNAL(triggered()), SIGNAL(aboutLicense()));
    connect(ui->actionAbout_Icons, SIGNAL(triggered()), SIGNAL(aboutIcons()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), SIGNAL(aboutQt()));
    ui->actionAbout_Qt->setMenuRole(QAction::AboutQtRole);

    //setFocusProxy(ui->modulesDock);
    ui->modulesDock->setFocusProxy(ui->moduleBrowser);
    ui->modulesDock->show();
    ui->modulesDock->raise();
    ui->modulesDock->setFocus();

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
            ui->actionQuit->setToolTip("Quit Vistle session");
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

void MainWindow::newHub(int hub, const QString &hubName, int nranks, const QString &address, int port,
                        const QString &logname, const QString &realname, bool hasUi, const QString &systype,
                        const QString &arch)
{
    m_moduleBrowser->addHub(hub, hubName, nranks, address, port, logname, realname, hasUi, systype, arch);
}

void MainWindow::deleteHub(int hub)
{
    m_moduleBrowser->removeHub(hub);
}

void MainWindow::moduleAvailable(int hub, const QString &mod, const QString &path, const QString &description)
{
    m_moduleBrowser->addModule(hub, mod, path, description);
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

    resize(settings.value("size", size()).toSize());
    move(settings.value("pos", pos()).toPoint());

    restoreState(settings.value("windowState").toByteArray());

    if (menuBar())
        menuBar()->setNativeMenuBar(settings.value("nativeMenuBar", true).toBool());

    settings.endGroup();
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");

    settings.setValue("size", size());
    settings.setValue("pos", pos());

    settings.setValue("windowState", saveState());

    if (menuBar())
        settings.setValue("nativeMenuBar", menuBar()->isNativeMenuBar());

    settings.endGroup();
}

} //namespace gui
