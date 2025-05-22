#ifndef VISTLE_GUI_SETNAMEDIALOG_H
#define VISTLE_GUI_SETNAMEDIALOG_H

#include <QDialog>

namespace Ui {
class SetNameDialog;
}

class SetNameDialog: public QDialog {
    Q_OBJECT

public:
    explicit SetNameDialog(QWidget *parent = nullptr);
    ~SetNameDialog();

private:
    Ui::SetNameDialog *ui;
};

#endif
