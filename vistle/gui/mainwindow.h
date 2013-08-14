#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "scene.h"
#include "handler.h"

#include <userinterface/userinterface.h>

#include <QObject>
#include <QList>
#include <QMainWindow>

namespace gui {

namespace Ui {
class MainWindow;

} //namespace Ui

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void printDebug(QString msg);
    Ui::MainWindow *ui;
    void setPrinter(StatePrinter *printer);
    void setRunner(UiRunner *runner);
    
private slots:
    void on_findButton_clicked();
    void on_dragButton_clicked();
    void on_sortButton_clicked();
    void on_invertModulesButton_clicked();

    void debug_msg(QString debugMsg);
    void newModule_msg(int moduleId, QString moduleName);
    void deleteModule_msg(int moduleId);
    void moduleStateChanged_msg(int moduleId, int stateBits, ModuleStatus modChangeType);
    void newParameter_msg(int moduleId, QString parameterName);
    void parameterValueChanged_msg(int moduleId, QString parameterName);
    void newPort_msg(int moduleId, QString portName);
    void newConnection_msg(int fromId, QString fromName,
                         int toId, QString toName);
    void deleteConnection_msg(int fromId, QString fromName,
                            int toId, QString toName);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    //void mousePressEvent(QMouseEvent *event);

private:
    //void loadTextFile();  ///\todo remove this
    QList<QString> loadModuleFile();
    void addModule(QString modName, QPointF dropPos);
    Scene *scene;
    ///\todo rename statePrinter
    StatePrinter *myPrinter;

};

} //namespace gui
#endif // MAINWINDOW_H
