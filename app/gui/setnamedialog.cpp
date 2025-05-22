#include "setnamedialog.h"
#include "ui_setnamedialog.h"

SetNameDialog::SetNameDialog(QWidget *parent): QDialog(parent), ui(new Ui::SetNameDialog)
{
    ui->setupUi(this);
}

SetNameDialog::~SetNameDialog()
{
    delete ui;
}
