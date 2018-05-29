#include "vistlebrowseredit.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QFileDialog>
#include <QToolButton>
#include <QAction>
#include <QDebug>
#include <QIcon>

#include "../remotefilebrowser/abstractfileinfogatherer.h"
#include "../remotefilebrowser/vistlefileinfogatherer.h"
#include "../remotefilebrowser/remotefileinfogatherer.h"
#include "../remotefilebrowser/remotefilesystemmodel.h"

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

    m_nameFilters << "All Files (*)";

    connect(m_button, &QToolButton::pressed, [this](){
        if (!m_browser) {
            if (m_ui) {
                m_fig = new VistleFileInfoGatherer(m_ui, m_moduleId);
            } else {
                m_fig = new RemoteFileInfoGatherer();
            }
            m_model = new RemoteFileSystemModel(m_fig);
            m_browser = new RemoteFileDialog(m_model);
            applyFileMode();
            applyNameFilters();

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
    delete m_model;
    delete m_fig;
}

void VistleBrowserEdit::setUi(vistle::UserInterface *ui)
{
    m_ui = ui;
}

QLineEdit *VistleBrowserEdit::edit() const
{
    return m_edit;
}

QString VistleBrowserEdit::text() const
{
    return m_edit->text();
}

void VistleBrowserEdit::setModuleId(int id)
{
    m_moduleId = id;
}

void VistleBrowserEdit::setText(const QString &text)
{
    m_edit->setText(text);
}

void VistleBrowserEdit::setFilters(const QString &filters)
{
    if (filters.isEmpty()) {
        m_nameFilters.clear();
        m_nameFilters << "All Files (*)";
    } else {
        m_nameFilters = filters.split("/");
    }
    applyNameFilters();
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

    switch (m_fileMode) {
    case File:
        m_browser->setFileMode(RemoteFileDialog::AnyFile);
        m_browser->setReadOnly(false);
        break;
    case ExistingFile:
        m_browser->setFileMode(RemoteFileDialog::ExistingFile);
        m_browser->setReadOnly(true);
        break;
    case Directory:
        m_browser->setFileMode(RemoteFileDialog::Directory);
        m_browser->setReadOnly(false);
        break;
    case ExistingDirectory:
        m_browser->setFileMode(RemoteFileDialog::Directory);
        m_browser->setReadOnly(true);
        break;
    }
#if 0
    if (m_fileMode == Directory || m_fileMode == ExistingDirectory) {
        m_browser->setOption(QFileDialog::ShowDirsOnly);
    } else {
        m_browser->setOption(QFileDialog::ShowDirsOnly, false);
    }
#endif
}

void VistleBrowserEdit::applyNameFilters()
{
    if (!m_browser)
        return;

    m_browser->setNameFilters(m_nameFilters);
}
