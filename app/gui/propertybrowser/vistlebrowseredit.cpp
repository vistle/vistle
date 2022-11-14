#include "vistlebrowseredit.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QAction>
#include <QDebug>
#include <QIcon>
#include <QApplication>
#include <QStyle>

#include "../remotefilebrowser/abstractfileinfogatherer.h"
#include "../remotefilebrowser/vistlefileinfogatherer.h"
#include "../remotefilebrowser/remotefileinfogatherer.h"
#include "../remotefilebrowser/remotefilesystemmodel.h"

VistleBrowserEdit::VistleBrowserEdit(QWidget *parent): QWidget(parent)
{
    m_layout = new QHBoxLayout(this);
    m_button = new QToolButton(this);
    m_button->setIcon(QIcon::fromTheme("folder", qApp->style()->standardIcon(QStyle::SP_DirClosedIcon)));
    m_edit = new QLineEdit(this);
    connect(m_edit, SIGNAL(editingFinished()), SIGNAL(editingFinished()));

    m_layout->addWidget(m_button);
    m_layout->addWidget(m_edit);

    m_nameFilters << "All Files (*)";

    m_title = "Vistle File Browser";

    connect(m_button, &QToolButton::pressed, [this]() {
        if (!m_browser) {
            if (m_ui) {
                m_fig = new VistleFileInfoGatherer(m_ui, m_moduleId);
            } else {
                m_fig = new RemoteFileInfoGatherer();
            }
            m_model = new RemoteFileSystemModel(m_fig);
            m_browser = new RemoteFileDialog(m_model);
            m_browser->setWindowTitle(m_title);
            applyFileMode();
            applyNameFilters();

            connect(m_browser, &QDialog::accepted, [this]() {
                const auto &files = m_browser->selectedFiles();
                if (!files.empty())
                    setText(files[0]);
                emit editingFinished();
            });

            connect(m_browser, &QDialog::finished, [this]() {
                m_button->setIcon(QIcon::fromTheme("folder", qApp->style()->standardIcon(QStyle::SP_DirClosedIcon)));
            });
        }

        if (m_browser->isHidden()) {
            m_browser->selectFile(text());
            m_browser->show();
            m_button->setIcon(QIcon::fromTheme("folder-open", qApp->style()->standardIcon(QStyle::SP_DirOpenIcon)));
        } else {
            m_browser->hide();
            m_button->setIcon(QIcon::fromTheme("folder", qApp->style()->standardIcon(QStyle::SP_DirClosedIcon)));
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

void VistleBrowserEdit::setTitle(const QString &title)
{
    m_title = title;
    if (m_browser)
        m_browser->setWindowTitle(title);
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

void VistleBrowserEdit::applyFileMode()
{
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
    if (m_fileMode == Directory || m_fileMode == ExistingDirectory) {
        m_browser->setOption(RemoteFileDialog::ShowDirsOnly);
    } else {
        m_browser->setOption(RemoteFileDialog::ShowDirsOnly, false);
    }
}

void VistleBrowserEdit::applyNameFilters()
{
    if (!m_browser)
        return;

    m_browser->setNameFilters(m_nameFilters);
}
