#include "vistlebrowseredit.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QFileDialog>
#include <QToolButton>
#include <QAction>
#include <QDebug>
#include <QIcon>

VistleBrowserEdit::VistleBrowserEdit(QWidget *parent)
: QWidget(parent)
{
    m_layout = new QHBoxLayout(this);
    m_button = new QToolButton(this);
    m_button->setIcon(QIcon::fromTheme("folder"));
    m_edit = new QLineEdit(this);
    connect(m_edit, SIGNAL(editingFinished()), SIGNAL(editingFinished()));

    m_layout->addWidget(m_button);
    m_layout->addWidget(m_edit);

    connect(m_button, &QToolButton::pressed, [this](){
        if (!m_browser) {
            m_browser = new QFileDialog;
            m_browser->setOption(QFileDialog::DontUseNativeDialog);
            applyFileMode();

            connect(m_browser, &QDialog::accepted, [this](){
                const auto &files = m_browser->selectedFiles();
                if (!files.empty())
                    setText(files[0]);
                emit editingFinished();
            });
        }

        if (m_browser->isHidden()) {
            if (m_browser->fileMode() == QFileDialog::Directory) {
                m_browser->setDirectory(text());
            } else {
                m_browser->selectFile(text());
            }
            m_browser->show();
        } else {
            m_browser->hide();
        }
    });

}

VistleBrowserEdit::~VistleBrowserEdit()
{
    delete m_browser;
}

QLineEdit *VistleBrowserEdit::edit() const
{
    return m_edit;
}

QString VistleBrowserEdit::text() const
{
    return m_edit->text();
}

void VistleBrowserEdit::setText(const QString &text)
{
    m_edit->setText(text);
}

void VistleBrowserEdit::setReadOnly(bool ro)
{
    m_edit->setReadOnly(ro);
}

const QValidator *VistleBrowserEdit::validator() const
{
    return m_edit->validator();
}

void VistleBrowserEdit::setValidator(QValidator *v)
{
    m_edit->setValidator(v);
}

void VistleBrowserEdit::setEchoMode(QLineEdit::EchoMode mode)
{
    m_edit->setEchoMode(mode);
}

void VistleBrowserEdit::setFileMode(VistleBrowserEdit::FileMode fileMode)
{
    m_fileMode = fileMode;
    applyFileMode();
}

void VistleBrowserEdit::applyFileMode() {
    if (!m_browser)
        return;

    m_browser->setFileMode(m_fileMode);
#if 0
    if (m_fileMode == QFileDialog::Directory) {
        m_browser->setOption(QFileDialog::ShowDirsOnly);
    } else {
        m_browser->setOption(QFileDialog::ShowDirsOnly, false);
    }
#endif
}
