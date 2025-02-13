#ifndef VISTLE_GUI_MODIFIEDDIALOG_H
#define VISTLE_GUI_MODIFIEDDIALOG_H

#include <QMessageBox>

namespace gui {
namespace Ui {
class ModifiedDialog;
}

class ModifiedDialog: public QMessageBox {
    Q_OBJECT

public:
    explicit ModifiedDialog(const QString &reason, QWidget *parent = 0);

signals:

public slots:
    virtual int exec();

private:
    QPushButton *m_discardButton;
};

} // namespace gui

#endif
