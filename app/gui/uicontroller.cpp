#ifdef HAVE_PYTHON
#include <vistle/python/pythoninterface.h>
#include <vistle/python/pythonmodule.h>
#endif
#include <vistle/core/messages.h>
#include "dataflownetwork.h"
#include "dataflowview.h"
#include "modifieddialog.h"
#include "modulebrowser.h"
#include "parameters.h"
#include "moduleview.h"
#include "uicontroller.h"
#include "vistleconsole.h"
#include <vistle/util/directory.h>

#include <thread>

#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QDockWidget>
#include <QGuiApplication>
#include <QStatusBar>
#include <QMenuBar>
#include <QUrl>
#include <QDesktopServices>

#include "ui_about.h"

namespace gui {

UiController::UiController(int argc, char *argv[], QObject *parent): QObject(parent), m_mainWindow(nullptr)
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

    if (argc > 2) {
        host = argv[1];
        port = atoi(argv[2]);
    } else if (argc > 1) {
        auto url = QUrl::fromUserInput(QString::fromLocal8Bit(argv[1]));
        host = url.host().toStdString();
        port = url.port(port);
    }

    m_mainWindow = new MainWindow();
    m_mainWindow->parameters()->setVistleObserver(&m_observer);
    m_mainWindow->setQuitOnExit(quitOnExit);

    m_ui.reset(new vistle::UserInterface(host, port, &m_observer));
#ifdef HAVE_PYTHON
    m_python.reset(new vistle::PythonInterface("Vistle GUI"));
#endif
    m_ui->registerObserver(&m_observer);
    m_vistleConnection.reset(new vistle::VistleConnection(*m_ui));
    m_vistleConnection->setQuitOnExit(quitOnExit);
#ifdef HAVE_PYTHON
    m_pythonAccess.reset(new vistle::UiPythonStateAccessor(m_vistleConnection.get()));
    m_pythonMod.reset(new vistle::PythonModule(*m_pythonAccess));
    vistle::Directory dir(argc, argv);
    m_pythonDir = dir.share();
#endif
    m_thread.reset(new std::thread(std::ref(*m_vistleConnection)));

    m_mainWindow->parameters()->setVistleConnection(m_vistleConnection.get());
    if (!m_ui->isConnected()) {
        std::cerr << "UI: not yet connected to " << host << ":" << port << std::endl;
    } else {
        m_mainWindow->enableConnectButton(false);
    }

    setCurrentFile(QString::fromStdString(m_ui->state().loadedWorkflowFile()));

    ///\todo declare the scene pointer in the header, then de-allocate in the destructor.
    m_scene = new DataFlowNetwork(m_vistleConnection.get(), m_mainWindow, m_mainWindow->dataFlowView());
    m_mainWindow->dataFlowView()->setScene(m_scene);
    m_mainWindow->dataFlowView()->addToToolBar(m_mainWindow->toolBar(), m_mainWindow->layerWidgetPosition());
    connect(m_mainWindow->dataFlowView(), SIGNAL(executeDataFlow()), SLOT(executeDataFlowNetwork()));
    connect(m_mainWindow->dataFlowView(), SIGNAL(visibleLayerChanged(int)), m_scene, SLOT(visibleLayerChanged(int)));

    connect(m_mainWindow, SIGNAL(quitRequested(bool &)), SLOT(quitRequested(bool &)));
    connect(m_mainWindow, SIGNAL(newDataFlow()), SLOT(clearDataFlowNetwork()));
    connect(m_mainWindow, SIGNAL(loadDataFlow()), SLOT(loadDataFlowNetwork()));
    connect(m_mainWindow, SIGNAL(saveDataFlow()), SLOT(saveDataFlowNetwork()));
    connect(m_mainWindow, SIGNAL(saveDataFlowAs()), SLOT(saveDataFlowNetworkAs()));
    connect(m_mainWindow, SIGNAL(executeDataFlow()), SLOT(executeDataFlowNetwork()));
    connect(m_mainWindow, SIGNAL(connectVistle()), SLOT(connectVistle()));
    connect(m_mainWindow, SIGNAL(showSessionUrl()), SLOT(showConnectionInfo()));
    connect(m_mainWindow, SIGNAL(copySessionUrl()), SLOT(copyConnectionInfo()));
    connect(m_mainWindow, SIGNAL(selectAllModules()), m_mainWindow->dataFlowView(), SLOT(selectAllModules()));
    connect(m_mainWindow, SIGNAL(selectInvert()), m_mainWindow->dataFlowView(), SLOT(selectInvert()));
    connect(m_mainWindow, SIGNAL(selectClear()), m_mainWindow->dataFlowView(), SLOT(selectClear()));
    connect(m_mainWindow, SIGNAL(selectSourceModules()), m_mainWindow->dataFlowView(), SLOT(selectSourceModules()));
    connect(m_mainWindow, SIGNAL(selectSinkModules()), m_mainWindow->dataFlowView(), SLOT(selectSinkModules()));
    connect(m_mainWindow, SIGNAL(deleteSelectedModules()), m_mainWindow->dataFlowView(), SLOT(deleteModules()));
    connect(m_mainWindow, SIGNAL(zoomOrig()), m_mainWindow->dataFlowView(), SLOT(zoomOrig()));
    connect(m_mainWindow, SIGNAL(zoomAll()), m_mainWindow->dataFlowView(), SLOT(zoomAll()));
    connect(m_mainWindow, SIGNAL(aboutQt()), SLOT(aboutQt()));
    connect(m_mainWindow, SIGNAL(aboutVistle()), SLOT(aboutVistle()));
    connect(m_mainWindow, SIGNAL(aboutLicense()), SLOT(aboutLicense()));
    connect(m_mainWindow, SIGNAL(aboutIcons()), SLOT(aboutIcons()));
    connect(m_mainWindow, SIGNAL(snapToGridChanged(bool)), m_mainWindow->dataFlowView(), SLOT(snapToGridChanged(bool)));

    connect(m_scene, SIGNAL(selectionChanged()), SLOT(moduleSelectionChanged()));

    connect(&m_observer, SIGNAL(quit_s()), qApp, SLOT(quit()));
    connect(&m_observer, SIGNAL(loadedWorkflowChanged_s(QString)), SLOT(setCurrentFile(QString)));
    connect(&m_observer, SIGNAL(sessionUrlChanged_s(QString)), SLOT(setSessionUrl(QString)));
    connect(&m_observer, SIGNAL(info_s(QString, int)), m_mainWindow->console(), SLOT(appendInfo(QString, int)));
    connect(&m_observer, SIGNAL(modified(bool)), m_mainWindow, SLOT(setModified(bool)));
    connect(&m_observer, SIGNAL(modified(bool)), this, SLOT(setModified(bool)));
    connect(&m_observer, SIGNAL(newModule_s(int, boost::uuids::uuid, QString)), m_scene,
            SLOT(addModule(int, boost::uuids::uuid, QString)));
    connect(&m_observer, SIGNAL(deleteModule_s(int)), m_scene, SLOT(deleteModule(int)));
    connect(&m_observer, SIGNAL(moduleStateChanged_s(int, int)), m_scene, SLOT(moduleStateChanged(int, int)));
    connect(&m_observer, &VistleObserver::message_s, [this](int senderId, int type, QString text) {
        if (m_scene)
            m_scene->moduleMessage(senderId, type, text);
        if (senderId == m_mainWindow->moduleView()->id()) {
            emit visibleModuleMessage(senderId, type, text);
        }
    });
    connect(m_mainWindow->moduleView(), SIGNAL(clearMessages(int)), m_scene, SLOT(clearMessages(int)));
    connect(m_mainWindow->moduleView(), SIGNAL(messagesVisibilityChanged(int, bool)), m_scene,
            SLOT(messagesVisibilityChanged(int, bool)));
    connect(this, SIGNAL(visibleModuleMessage(int, int, QString)), m_mainWindow->moduleView(),
            SLOT(appendMessage(int, int, QString)));
    connect(&m_observer, SIGNAL(itemInfo_s(QString, int, int, QString)), m_scene,
            SLOT(itemInfoChanged(QString, int, int, QString)));
    connect(&m_observer, SIGNAL(newPort_s(int, QString)), m_scene, SLOT(newPort(int, QString)));
    connect(&m_observer, SIGNAL(deletePort_s(int, QString)), m_scene, SLOT(deletePort(int, QString)));
    connect(&m_observer, SIGNAL(newConnection_s(int, QString, int, QString)), m_scene,
            SLOT(newConnection(int, QString, int, QString)));
    connect(&m_observer, SIGNAL(deleteConnection_s(int, QString, int, QString)), m_scene,
            SLOT(deleteConnection(int, QString, int, QString)));

    connect(&m_observer, SIGNAL(newParameter_s(int, QString)), SLOT(newParameter(int, QString)));
    connect(&m_observer, SIGNAL(parameterValueChanged_s(int, QString)), SLOT(parameterValueChanged(int, QString)));

    connect(&m_observer, SIGNAL(newHub_s(int, QString, int, QString, int, QString, QString, bool, QString, QString)),
            m_mainWindow, SLOT(newHub(int, QString, int, QString, int, QString, QString, bool, QString, QString)));
    connect(&m_observer, SIGNAL(deleteHub_s(int)), m_mainWindow, SLOT(deleteHub(int)));
    connect(&m_observer, SIGNAL(moduleAvailable_s(int, QString, QString, QString, QString)), m_mainWindow,
            SLOT(moduleAvailable(int, QString, QString, QString, QString)));

    connect(&m_observer, SIGNAL(status_s(int, QString, int)), SLOT(statusUpdated(int, QString, int)));
    connect(&m_observer, SIGNAL(moduleStatus_s(int, QString, int)), m_scene, SLOT(moduleStatus(int, QString, int)));

    connect(&m_observer, SIGNAL(screenshot_s(QString, bool)), this, SLOT(screenshot(QString, bool)));

    QObject::connect(m_mainWindow->m_moduleBrowser, &ModuleBrowser::startModule, this,
                     [this](int hubId, const QString &moduleName, Qt::Key direction) {
                         m_scene->addModule(hubId, moduleName, direction);
                     });
    connect(m_mainWindow->m_console, &QConsole::anchorClicked, [this](QString link) {
        bool isNumber = false;
        int id = link.toInt(&isNumber);
        if (isNumber) {
            if (vistle::message::Id::isModule(id)) {
                m_scene->clearSelection();
                if (auto mod = m_scene->findModule(id)) {
                    mod->setSelected(true);
                }
            }
        } else if (link.startsWith("vistle:")) {
            qApp->clipboard()->setText(link);
            m_mainWindow->statusBar()->showMessage("Link copied to clipboard", 3000);
        } else {
            QDesktopServices::openUrl(link);
        }
    });

    connect(m_mainWindow->m_moduleBrowser, &ModuleBrowser::requestRemoveHub, [this](int id) {
        vistle::message::Quit quit(id);
        m_vistleConnection->sendMessage(quit);
    });
    connect(m_mainWindow->m_moduleBrowser, &ModuleBrowser::requestAttachDebugger, [this](int id) {
        assert(vistle::message::Id::isHub(id));
        vistle::message::Debug dbg(id);
        dbg.setDestId(id);
        m_vistleConnection->sendMessage(dbg);
    });
    connect(m_mainWindow->m_moduleBrowser, &ModuleBrowser::showStatusTip,
            [this](const QString &tip) { m_mainWindow->statusBar()->showMessage(tip, 3000); });

    connect(m_mainWindow->moduleView(), &ModuleView::executeModule, [this](int id) {
        auto mod = m_scene->findModule(id);
        if (mod) {
            mod->execModule();
        }
    });
    connect(m_mainWindow->moduleView(), &ModuleView::deleteModule, [this](int id) {
        auto mod = m_scene->findModule(id);
        if (mod) {
            mod->deleteModule();
        }
    });

    m_mainWindow->show();
}

void UiController::init()
{
#ifdef HAVE_PYTHON
    m_python->init();
#endif
    m_mainWindow->m_console->init();
#ifdef HAVE_PYTHON
    m_pythonMod->import(&vistle::PythonInterface::the().nameSpace(), m_pythonDir);
#endif

    m_mainWindow->dataFlowView()->snapToGridChanged(m_mainWindow->isSnapToGrid());

    moduleSelectionChanged();
}

UiController::~UiController()
{}

void UiController::finish()
{
    m_mainWindow->dataFlowView()->setScene(nullptr);

    delete m_scene;
    m_scene = nullptr;

    m_vistleConnection->cancel();
    m_thread->join();
    m_thread.reset();

    m_mainWindow->m_console->finish();

#ifdef HAVE_PYTHON
    m_pythonMod.reset();
    m_python.reset();
#endif

    delete m_mainWindow;
    m_mainWindow = nullptr;

    m_ui.reset();
    m_vistleConnection.reset();
}

void UiController::quitRequested(bool &allowed)
{
    allowed = checkModified("Quit");
}

bool UiController::checkModified(const QString &reason)
{
    //std::cerr << "modification count: " << m_observer->modificationCount() << std::endl;
    if (m_observer.modificationCount() == 0 && !m_modified)
        return true;

    ModifiedDialog d(reason, m_mainWindow);

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
    m_observer.resetModificationCount();
    setCurrentFile(QString());
}

void UiController::loadDataFlowNetwork()
{
    if (!checkModified("Open"))
        return;

    QString dir = m_currentFile.isEmpty() ? QDir::currentPath() : m_currentFile;
    QString filename = QFileDialog::getOpenFileName(m_mainWindow, tr("Open Data Flow Network"), dir,
                                                    tr("Vistle files (*.vsl);;Python files (*.py);;All files (*)"));

    if (filename.isEmpty())
        return;

    m_observer.resetModificationCount();
    clearDataFlowNetwork();

#ifdef HAVE_PYTHON
    vistle::PythonInterface::the().exec_file(filename.toStdString());
#endif

    setCurrentFile(filename);
    m_observer.resetModificationCount();
}

void UiController::saveDataFlowNetwork(const QString &filename)
{
    if (filename.isEmpty()) {
        if (!m_currentFile.isEmpty())
            saveDataFlowNetwork(m_currentFile);
        else
            saveDataFlowNetworkAs();
    } else {
        std::cerr << "writing to " << filename.toStdString() << std::endl;
        setCurrentFile(filename);
        std::string cmd = "save(\"";
        cmd += filename.toStdString();
        cmd += "\")";
#ifdef HAVE_PYTHON
        vistle::PythonInterface::the().exec(cmd);
#endif
        m_observer.resetModificationCount();
    }
}

void UiController::saveDataFlowNetworkAs(const QString &filename)
{
    QString newFile = QFileDialog::getSaveFileName(m_mainWindow, tr("Save Data Flow Network"),
                                                   filename.isEmpty() ? QDir::currentPath() : filename,
                                                   tr("Vistle files (*.vsl);;Python files (*.py);;All files (*)"));

    if (!newFile.isEmpty()) {
        saveDataFlowNetwork(newFile);
    }
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
            if (selectedModules.size() > 1)
                break;
        }
    }

    QString title = "Module Parameters";
    if (selectedModules.size() == 1) {
        const Module *m = selectedModules[0];
        auto id = m->id();
        m_mainWindow->parameters()->setModule(id);
        title = QString("Parameters: %1").arg(m->name());
        m_mainWindow->parameters()->adjustSize();
        m_mainWindow->parameters()->show();
        m_mainWindow->moduleViewDock()->show();
        m_mainWindow->moduleViewDock()->raise();
        m_mainWindow->moduleView()->setId(id);
        if (Module *m = m_scene->findModule(id)) {
            m_mainWindow->moduleView()->setMessages(m->messages(), m->messagesVisible());
        }
    } else {
        m_mainWindow->parameters()->setModule(vistle::message::Id::Vistle);
        m_mainWindow->moduleView()->setId(vistle::message::Id::Vistle);
        title = QString("Session Parameters");
        m_mainWindow->parameters()->adjustSize();
        m_mainWindow->modulesDock()->show();
        m_mainWindow->modulesDock()->raise();
    }
    m_mainWindow->moduleViewDock()->setWindowTitle(title);
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
            auto vp = std::dynamic_pointer_cast<vistle::VectorParameter>(p);
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
        auto p = vistle::VistleConnection::the().getParameter(moduleId, "_position");
        auto vp = std::dynamic_pointer_cast<vistle::VectorParameter>(p);
        if (vp && !vp->isDefault()) {
            vistle::ParamVector pos = vp->getValue();
            m_scene->moveModule(moduleId, pos[0], pos[1]);
        }
    }
    if (parameterName == "_layer") {
        auto p = vistle::VistleConnection::the().getParameter(moduleId, "_layer");
        auto l = std::dynamic_pointer_cast<vistle::IntParameter>(p);
        if (l) {
            const int layer = l->getValue();
            if (layer >= DataFlowView::the()->numLayers()) {
                DataFlowView::the()->setNumLayers(layer + 1);
            }
            if (!l->isDefault()) {
                if (Module *m = m_scene->findModule(moduleId)) {
                    m->setLayer(layer);
                }
            }
        }
    }
}

void UiController::statusUpdated(int id, QString text, int prio)
{
    if (m_mainWindow) {
        if (text.isEmpty())
            m_mainWindow->statusBar()->clearMessage();
        else
            m_mainWindow->statusBar()->showMessage(text);
    }
}

void UiController::setCurrentFile(QString file)
{
    m_currentFile = file;
    m_mainWindow->setFilename(m_currentFile);
}

void UiController::setSessionUrl(QString url)
{
    m_sessionUrl = url;
    showConnectionInfo();
}

void UiController::setModified(bool modified)
{
    m_modified = modified;
}

void UiController::about(const char *title, const char *filename)
{
    QDialog *aboutDialog = new QDialog;
    ::Ui::AboutDialog *ui = new ::Ui::AboutDialog;
    ui->setupUi(aboutDialog);
    aboutDialog->setWindowTitle(title);

    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QString data(file.readAll());
        if (QString(filename).endsWith(".md")) {
#if QT_VERSION >= 0x051400
            ui->textEdit->setMarkdown(data);
#else
            ui->textEdit->setPlainText(data);
#endif
        } else {
            ui->textEdit->setPlainText(data);
        }
    }
    ui->textEdit->setSearchPaths(QStringList{":/aboutData"});
    ui->textEdit->setWordWrapMode(QTextOption::WordWrap);
#if 0
    connect(ui->textEdit, &QTextBrowser::sourceChanged, [&ui](const QUrl &url) {
        bool isText = url.path().endsWith(".txt");
        ui->textEdit->setWordWrapMode(isText ? QTextOption::NoWrap : QTextOption::WordWrap);
    });
#endif

    QString text("Welcome to scientific visualization with <a href='https://vistle.io/'>Vistle</a>!");
    text.append("<br>You can follow Vistle development on <a href='https://github.com/vistle/vistle'>GitHub</a>.");

    ui->label->setText(text);
    ui->label->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::LinksAccessibleByMouse);
    ui->label->setOpenExternalLinks(true);

    aboutDialog->exec();
}

void UiController::aboutVistle()
{
    about("About Vistle", ":/aboutData/Vistle.md");
}

void UiController::aboutLicense()
{
    about("License", ":/aboutData/LICENSE.txt");
}

void UiController::aboutIcons()
{
    about("Font Awesome Icons", ":/aboutData/icons.md");
}

void UiController::aboutQt()
{
    QMessageBox::aboutQt(new QDialog, "Qt");
}

void UiController::showConnectionInfo()
{
#if 1
    m_mainWindow->m_console->appendInfo(QString("Share this: <a href=\"%1\">%1</a>").arg(m_sessionUrl),
                                        vistle::message::SendText::Info);
#else
    m_mainWindow->m_console->appendInfo(QString("Share this: %1").arg(m_sessionUrl), vistle::message::SendText::Info);
#endif
}

void UiController::copyConnectionInfo()
{
    QGuiApplication::clipboard()->setText(m_sessionUrl);
}

void UiController::screenshot(QString imageFile, bool quit)
{
    m_mainWindow->dataFlowView()->snapshot(imageFile);
    if (quit) {
        vistle::message::Quit q;
        m_vistleConnection->sendMessage(q);
    }
}

} // namespace gui
