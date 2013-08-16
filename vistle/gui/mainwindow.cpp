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

#include "qconsole/qpyconsole.h"

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

    m_console = new VistleConsole(this, "Welcome to Vistle!");
    ui->consoleWidget->setWidget(m_console);
    setFocusProxy(m_console);
    ui->drawArea->setAttribute(Qt::WA_AlwaysShowToolTips);
    ui->drawArea->setDragMode(QGraphicsView::RubberBandDrag);

    ///\todo declare the scene pointer in the header, then de-allocate in the destructor.
    //QGraphicsScene *scene = new QGraphicsScene(this);
    scene = new Scene(ui->drawArea);

    // load a text file containing all the modules in vistle.
    ///\todo loadModuleFile() returns a list of modules, pipe this to the scene
    auto modules = loadModuleFile();
    scene->setModules(modules);
    for (const QString &m: modules)
       ui->moduleListWidget->addItem(m);
    ui->drawArea->setScene(scene);
    ui->drawArea->show();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete scene;
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
    QString text = "Module started: " + moduleName + " with ID: " + QString::number(moduleId) + "\n";
    scene->addModule(moduleId, spawnUuid, moduleName);
    m_console->append(text);
}

void MainWindow::deleteModule_msg(int moduleId)
{
    QString text = "Module deleted: " + QString::number(moduleId) + "\n";
    m_console->append(text);
}

void MainWindow::moduleStateChanged_msg(int moduleId, int stateBits, ModuleStatus modChangeType)
{
   if (Module *m = scene->findModule(moduleId)) {
      m->setStatus(modChangeType);
   }
    QString text;
    if (modChangeType == INITIALIZED) text = "Module state change on ID: " + QString::number(moduleId) + " initialized\n";
    if (modChangeType == KILLED) text = "Module state change on ID: " + QString::number(moduleId) + " killed\n";
    if (modChangeType == BUSY) text = "Module state change on ID: " + QString::number(moduleId) + " busy\n";

    m_console->append(text);
}

void MainWindow::newParameter_msg(int moduleId, QString parameterName)
{
    QString text = "New parameter on ID: " + QString::number(moduleId) + ":" + parameterName + "\n";
    m_console->append(text);
}

void MainWindow::parameterValueChanged_msg(int moduleId, QString parameterName)
{
    QString text = "Parameter value changed on ID: " + QString::number(moduleId) + ":" + parameterName + "\n";
    m_console->append(text);
    if (parameterName == "_x" || parameterName == "_y") {
       if (Module *m = scene->findModule(moduleId)) {
          vistle::Parameter *p = VistleConnection::the().getParameter(moduleId, parameterName);
          if (vistle::FloatParameter *fp = dynamic_cast<vistle::FloatParameter *>(p)) {
             double val = fp->getValue();
             if (parameterName == "_x")
                m->setX(val);
             else if (parameterName == "_y")
                m->setY(val);
          }
       }
    }
}

void MainWindow::newPort_msg(int moduleId, QString portName)
{
    QString text = "New port on ID: " + QString::number(moduleId) + ":" + portName + "\n";
    m_console->append(text);
}

void MainWindow::newConnection_msg(int fromId, QString fromName,
                                   int toId, QString toName)
{
    QString text = "New Connection: " + QString::number(fromId) + ":" + fromName + " -> " + QString::number(toId) + ":" + toName + "\n";
    m_console->append(text);
}

void MainWindow::deleteConnection_msg(int fromId, QString fromName,
                                      int toId, QString toName)
{
    QString text = "Connection removed: " + QString::number(fromId) + ":" + fromName + " -> " + QString::number(toId) + ":" + toName + "\n";
    m_console->append(text);
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
    connect(m_observer, SIGNAL(moduleStateChanged_s(int, int, ModuleStatus)),
            this, SLOT(moduleStateChanged_msg(int, int, ModuleStatus)));
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
}

void MainWindow::setVistleConnection(VistleConnection *runner)
{
    scene->setRunner(runner);
    m_vistleConnection = runner;
}

/*!
 * \brief MainWindow::on_findButton_clicked find button action for the text search
 */
void MainWindow::on_findButton_clicked()
{
    QString searchString = ui->lineEdit->text();
    ui->textEdit->find(searchString, QTextDocument::FindWholeWords);
    QMessageBox msgBox;
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start("find /mnt/raid/home/hpcmaumu/steve/Sandbox/ -name input.txt");
    proc.waitForFinished();
    QByteArray out = proc.readAllStandardOutput();

    msgBox.setText(out);
    msgBox.exec();
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
    scene->sortModules();
}

/*!
 * \brief MainWindow::on_invertModulesButton_clicked button that inverts the orientation of the modules in the scene
 */
void MainWindow::on_invertModulesButton_clicked()
{
    scene->invertModules();
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

/*!
 * \brief MainWindow::dragEnterEvent
 * \param event
 *
 * \todo clean up the distribution of event handling.
 */
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

/*!
 * \brief MainWindow::dropEvent takes action on a drop event anywhere in the window.
 *
 * This drop event method is currently the only drop event in the program. It looks for drops into the
 * QGraphicsView (drawArea), and calls the scene to add a module depending on the position.
 *
 * \param event
 * \todo clarify all the event handling, and program the creation and handling of events more elegantly.
 * \todo put information into the event, to remove the need to have drag events in the mainWindow
 */
void MainWindow::dropEvent(QDropEvent *event)
{
    // test for dropping in the actual draw area.
    QRect widgetRect = ui->drawArea->geometry();
    QString moduleName;
    if(widgetRect.contains(event->pos())) {
        // This is probably the most obtuse way possible to map coordinates, but it mostly works.
        // Maps the coordinates in the following way:
        //  drop location -> drawArea -> scene -> graphicsObject
        QPoint refPos = QPoint(event->pos().x(), event->pos().y());
        QPointF newPos = ui->drawArea->mapFromParent(refPos);
        refPos.setX(newPos.x());
        refPos.setY(newPos.y());
        newPos = ui->drawArea->mapToScene(refPos);

        ///\todo this solution for module name works only if an item is selected in the list,
        /// and fails for any other drop event. Implement MIME handling on events, like in mapEditor
        moduleName = ui->moduleListWidget->currentItem()->text();
        scene->addModule(moduleName, newPos);

    }
}

} //namespace gui
