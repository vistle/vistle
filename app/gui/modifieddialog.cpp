#include "modifieddialog.h"

#include <QPushButton>
#include <QDialogButtonBox>

#include <cassert>

namespace gui {

ModifiedDialog::ModifiedDialog(const QString &reason, QWidget *parent): QMessageBox(parent), m_discardButton(nullptr)
{
    setWindowModality(Qt::WindowModal);

    setIcon(QMessageBox::Question);

    setWindowTitle(QString("%1").arg(reason));

    setText("The workflow has been modified.");
    if (reason == "Open") {
        setInformativeText("You will lose your changes, if you load another workflow without saving.");
    } else if (reason == "New") {
        setInformativeText("You will lose your changes, if you clear the workflow without saving.");
    } else if (reason == "Quit") {
        setInformativeText("You will lose your changes, if you quit without saving.");
    } else {
        setInformativeText("You will lose your changes, if you continue without saving.");
    }

    setStandardButtons(QMessageBox::Cancel | QMessageBox::Save);
    m_discardButton = addButton(reason, QMessageBox::DestructiveRole);
    setDefaultButton(m_discardButton);
}

int ModifiedDialog::exec()
{
    int result = QMessageBox::exec();
    if (result == QMessageBox::Cancel)
        return result;
    if (result == QMessageBox::Save)
        return result;

    if (clickedButton() == m_discardButton)
        return QMessageBox::Discard;

    assert("QMessageBox result not handled" == 0);
    return QMessageBox::Cancel;
}

} // namespace gui
