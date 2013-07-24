#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QtCore>
#include <QtWidgets>
#include "vscene.h"

namespace Ui {
class MainWindow;

}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_findButton_clicked();
    void on_dragButton_clicked();
    void on_sortButton_clicked();

    void on_invertModulesButton_clicked();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    //void mousePressEvent(QMouseEvent *event);

private:
    Ui::MainWindow *ui;
    void loadTextFile();
    void addModule(QString modName, QPointF dropPos);
    VScene *scene;

};

#endif // MAINWINDOW_H
