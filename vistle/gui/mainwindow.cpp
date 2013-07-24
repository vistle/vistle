#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    loadTextFile();
    ui->messageDockWidget->setWidget(ui->messageBox);
    ui->drawArea->setAttribute(Qt::WA_AlwaysShowToolTips);
    ui->drawArea->setDragMode(QGraphicsView::RubberBandDrag);

    ///\todo declare the scene pointer in the header, then de-allocate in the destructor.
    //QGraphicsScene *scene = new QGraphicsScene(this);
    scene = new VScene(ui->drawArea);
    ui->drawArea->setScene(scene);
    ui->drawArea->show();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete scene;
    ///\todo Keep track of all things created with new:
    /// VModules; scene; QDrag; mimeData;
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
 * \brief MainWindow::loadTextFile load and store a text file
 */
void MainWindow::loadTextFile()
{
    QString root = "/mnt/raid/home/hpcskimb/Sandbox/exploratory/input.txt";
    QFile inputFile(root);
    inputFile.open(QIODevice::ReadOnly);

    QTextStream in(&inputFile);
    QString line = in.readAll();
    inputFile.close();

    ui->textEdit->setPlainText(line);
    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1);
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
 * This drop event method is currently the only one in the program. It looks for drops into the
 * QGraphicsView (drawArea), and calls the scene to add a module depending on the position.
 *
 * \param event
 * \todo clarify all the event handling, and program the creation and handling of events more elegantly.
 */
void MainWindow::dropEvent(QDropEvent *event)
{
    // test for dropping in the actual draw area.
    QRect widgetRect = ui->drawArea->geometry();
    if(widgetRect.contains(event->pos())) {
        // This is probably the most obtuse way possible to map coordinates, but it (mostly) works.
        // Maps the coordinates in the following way:
        //  drop location -> drawArea -> scene -> graphicsObject
        QPoint refPos = QPoint(event->pos().x(), event->pos().y());
        QPointF newPos = ui->drawArea->mapFromParent(refPos);
        refPos.setX(newPos.x());
        refPos.setY(newPos.y());
        newPos = ui->drawArea->mapToScene(refPos);

        scene->addModule(event->source()->objectName(), newPos);

    }

    ///\todo should the standard method implementation be called?
}
