/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#define QT_NO_URL_CAST_FROM_STRING

#include <qvariant.h>
//#include <private/qwidgetitemdata_p.h>
#include "remotefiledialog.h"
#include "abstractfilesystemmodel.h"
#include "localfilesystemmodel.h"
#include "remotefilesystemmodel.h"
#include "remotefileinfogatherer.h"
#include "vistlefileinfogatherer.h"

//#include "qpa/qplatformdialoghelper.h"

#include "filedialoghelper.h"

#include "remotefiledialog_p.h"
//#include <private/qguiapplication_p.h>
#include <qfontmetrics.h>
#include <qaction.h>
#include <qheaderview.h>
#include <qshortcut.h>
#include <qgridlayout.h>
#if QT_CONFIG(menu)
#include <qmenu.h>
#endif
#if QT_CONFIG(messagebox)
#include <qmessagebox.h>
#endif
#include <stdlib.h>
#include <qsettings.h>
#include <qdebug.h>
#include <qmimedatabase.h>
#include <qapplication.h>
#include <qstylepainter.h>
#include "ui_remotefiledialog.h"
#if defined(Q_OS_UNIX)
#include <pwd.h>
#include <unistd.h> // for pathconf() on OS X
#elif defined(Q_OS_WIN)
#include <QtCore/qt_windows.h>
#endif

#include "remotefileiconprovider.h"
#include "qpushbutton.h"
#include <memory>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QUrl, lastVisitedDir)

namespace {
static const char *remoteFilterRegExp = "^(.*)\\(([a-zA-Z0-9_.,*? +;#\\-\\[\\]@\\{\\}/!<>\\$%&=^~:\\|]*)\\)$";

// Makes a list of filters from a normal filter string "Image Files (*.png *.jpg)"
QStringList cleanFilterList(const QString &filter)
{
    QRegExp regexp(QString::fromLatin1(remoteFilterRegExp));
    Q_ASSERT(regexp.isValid());
    QString f = filter;
    int i = regexp.indexIn(f);
    if (i >= 0)
        f = regexp.cap(2);
    return f.split(QLatin1Char(' '), QString::SkipEmptyParts);
}

} // namespace

/*!
  \class RemoteFileDialog
  \brief The RemoteFileDialog class provides a dialog that allow users to select files or directories.
  \ingroup standard-dialogs
  \inmodule QtWidgets

  The RemoteFileDialog class enables a user to traverse the file system in
  order to select one or many files or a directory.

  The easiest way to create a RemoteFileDialog is to use the static functions.

  \snippet code/src_gui_dialogs_qfiledialog.cpp 0

  In the above example, a modal RemoteFileDialog is created using a static
  function. The dialog initially displays the contents of the "/home/jana"
  directory, and displays files matching the patterns given in the
  string "Image Files (*.png *.jpg *.bmp)". The parent of the file dialog
  is set to \e this, and the window title is set to "Open Image".

  If you want to use multiple filters, separate each one with
  \e two semicolons. For example:

  \snippet code/src_gui_dialogs_qfiledialog.cpp 1

  You can create your own RemoteFileDialog without using the static
  functions. By calling setFileMode(), you can specify what the user must
  select in the dialog:

  \snippet code/src_gui_dialogs_qfiledialog.cpp 2

  In the above example, the mode of the file dialog is set to
  AnyFile, meaning that the user can select any file, or even specify a
  file that doesn't exist. This mode is useful for creating a
  "Save As" file dialog. Use ExistingFile if the user must select an
  existing file, or \l Directory if only a directory may be selected.
  See the \l RemoteFileDialog::FileMode enum for the complete list of modes.

  The fileMode property contains the mode of operation for the dialog;
  this indicates what types of objects the user is expected to select.
  Use setNameFilter() to set the dialog's file filter. For example:

  \snippet code/src_gui_dialogs_qfiledialog.cpp 3

  In the above example, the filter is set to \c{"Images (*.png *.xpm *.jpg)"},
  this means that only files with the extension \c png, \c xpm,
  or \c jpg will be shown in the RemoteFileDialog. You can apply
  several filters by using setNameFilters(). Use selectNameFilter() to select
  one of the filters you've given as the file dialog's default filter.

  The file dialog has two view modes: \l{RemoteFileDialog::}{List} and
  \l{RemoteFileDialog::}{Detail}.
  \l{RemoteFileDialog::}{List} presents the contents of the current directory
  as a list of file and directory names. \l{RemoteFileDialog::}{Detail} also
  displays a list of file and directory names, but provides additional
  information alongside each name, such as the file size and modification
  date. Set the mode with setViewMode():

  \snippet code/src_gui_dialogs_qfiledialog.cpp 4

  The last important function you will need to use when creating your
  own file dialog is selectedFiles().

  \snippet code/src_gui_dialogs_qfiledialog.cpp 5

  In the above example, a modal file dialog is created and shown. If
  the user clicked OK, the file they selected is put in \c fileName.

  The dialog's working directory can be set with setDirectory().
  Each file in the current directory can be selected using
  the selectFile() function.

  The \l{dialogs/standarddialogs}{Standard Dialogs} example shows
  how to use RemoteFileDialog as well as other built-in Qt dialogs.

  By default, a platform-native file dialog will be used if the platform has
  one. In that case, the widgets which would otherwise be used to construct the
  dialog will not be instantiated, so related accessors such as layout() and
  itemDelegate() will return null. You can set the \l DontUseNativeDialog option to
  ensure that the widget-based implementation will be used instead of the
  native dialog.

  \sa QDir, QFileInfo, QFile, QColorDialog, QFontDialog, {Standard Dialogs Example},
      {Application Example}
*/

/*!
    \enum RemoteFileDialog::AcceptMode

    \value AcceptOpen
    \value AcceptSave
*/

/*!
    \enum RemoteFileDialog::ViewMode

    This enum describes the view mode of the file dialog; i.e. what
    information about each file will be displayed.

    \value Detail Displays an icon, a name, and details for each item in
                  the directory.
    \value List   Displays only an icon and a name for each item in the
                  directory.

    \sa setViewMode()
*/

/*!
    \enum RemoteFileDialog::FileMode

    This enum is used to indicate what the user may select in the file
    dialog; i.e. what the dialog will return if the user clicks OK.

    \value AnyFile        The name of a file, whether it exists or not.
    \value ExistingFile   The name of a single existing file.
    \value Directory      The name of a directory. Both files and
                          directories are displayed. However, the native Windows
                          file dialog does not support displaying files in the
                          directory chooser.
    \value ExistingFiles  The names of zero or more existing files.

    This value is obsolete since Qt 4.5:

    \sa setFileMode()
*/

/*!
    \enum RemoteFileDialog::Option

    \value ShowDirsOnly Only show directories in the file dialog. By
    default both files and directories are shown. (Valid only in the
    \l Directory file mode.)

    \value DontResolveSymlinks Don't resolve symlinks in the file
    dialog. By default symlinks are resolved.

    \value DontConfirmOverwrite Don't ask for confirmation if an
    existing file is selected.  By default confirmation is requested.

    \value DontUseNativeDialog Don't use the native file dialog. By
    default, the native file dialog is used unless you use a subclass
    of RemoteFileDialog that contains the Q_OBJECT macro, or the platform
    does not have a native dialog of the type that you require.

    \value ReadOnly Indicates that the model is readonly.

    \value HideNameFilterDetails Indicates if the file name filter details are
    hidden or not.

    \value DontUseSheet In previous versions of Qt, the static
    functions would create a sheet by default if the static function
    was given a parent. This is no longer supported and does nothing in Qt 4.5, The
    static functions will always be an application modal dialog. If
    you want to use sheets, use RemoteFileDialog::open() instead.

    \value DontUseCustomDirectoryIcons Always use the default directory icon.
    Some platforms allow the user to set a different icon. Custom icon lookup
    cause a big performance impact over network or removable drives.
    Setting this will enable the RemoteFileIconProvider::DontUseCustomDirectoryIcons
    option in the icon provider. This enum value was added in Qt 5.2.
*/

/*!
  \enum RemoteFileDialog::DialogLabel

  \value LookIn
  \value FileName
  \value FileType
  \value Accept
  \value Reject
*/

/*!
    \fn void RemoteFileDialog::filesSelected(const QStringList &selected)

    When the selection changes for local operations and the dialog is
    accepted, this signal is emitted with the (possibly empty) list
    of \a selected files.

    \sa currentChanged(), QDialog::Accepted
*/

/*!
    \fn void RemoteFileDialog::urlsSelected(const QList<QUrl> &urls)

    When the selection changes and the dialog is accepted, this signal is
    emitted with the (possibly empty) list of selected \a urls.

    \sa currentUrlChanged(), QDialog::Accepted
    \since 5.2
*/

/*!
    \fn void RemoteFileDialog::fileSelected(const QString &file)

    When the selection changes for local operations and the dialog is
    accepted, this signal is emitted with the (possibly empty)
    selected \a file.

    \sa currentChanged(), QDialog::Accepted
*/

/*!
    \fn void RemoteFileDialog::urlSelected(const QUrl &url)

    When the selection changes and the dialog is accepted, this signal is
    emitted with the (possibly empty) selected \a url.

    \sa currentUrlChanged(), QDialog::Accepted
    \since 5.2
*/

/*!
    \fn void RemoteFileDialog::currentChanged(const QString &path)

    When the current file changes for local operations, this signal is
    emitted with the new file name as the \a path parameter.

    \sa filesSelected()
*/

/*!
    \fn void RemoteFileDialog::currentUrlChanged(const QUrl &url)

    When the current file changes, this signal is emitted with the
    new file URL as the \a url parameter.

    \sa urlsSelected()
    \since 5.2
*/

/*!
  \fn void RemoteFileDialog::directoryEntered(const QString &directory)
  \since 4.3

  This signal is emitted for local operations when the user enters
  a \a directory.
*/

/*!
  \fn void RemoteFileDialog::directoryUrlEntered(const QUrl &directory)

  This signal is emitted when the user enters a \a directory.

  \since 5.2
*/

/*!
  \fn void RemoteFileDialog::filterSelected(const QString &filter)
  \since 4.3

  This signal is emitted when the user selects a \a filter.
*/

QT_BEGIN_INCLUDE_NAMESPACE
#include <QMetaEnum>
#include <qshortcut.h>
QT_END_INCLUDE_NAMESPACE

/*!
    \fn RemoteFileDialog::RemoteFileDialog(QWidget *parent, Qt::WindowFlags flags)

    Constructs a file dialog with the given \a parent and widget \a flags.
*/
RemoteFileDialog::RemoteFileDialog(AbstractFileSystemModel *model, QWidget *parent, Qt::WindowFlags f)
: QDialog(parent, f), vd_ptr(new RemoteFileDialogPrivate(this)), m_model(model)
{
    Q_D(RemoteFileDialog);
    d->init();
}

/*!
    Constructs a file dialog with the given \a parent and \a caption that
    initially displays the contents of the specified \a directory.
    The contents of the directory are filtered before being shown in the
    dialog, using a semicolon-separated list of filters specified by
    \a filter.
*/
RemoteFileDialog::RemoteFileDialog(AbstractFileSystemModel *model, QWidget *parent, const QString &caption,
                                   const QString &directory, const QString &filter)
: QDialog(parent, 0), vd_ptr(new RemoteFileDialogPrivate(this)), m_model(model)
{
    Q_D(RemoteFileDialog);
    d->init(QUrl::fromLocalFile(directory), filter, caption);
}

/*!
    \internal
*/
RemoteFileDialog::RemoteFileDialog(const RemoteFileDialogArgs &args)
: QDialog(args.parent, 0), vd_ptr(new RemoteFileDialogPrivate(this))
{
    Q_D(RemoteFileDialog);
    d->init(args.directory, args.filter, args.caption);
    setFileMode(args.mode);
    setOptions(args.options);
    selectFile(args.selection);
}

/*!
    Destroys the file dialog.
*/
RemoteFileDialog::~RemoteFileDialog()
{
#ifndef QT_NO_SETTINGS
    Q_D(RemoteFileDialog);
    d->saveSettings();
#endif
}

/*!
    \since 4.3
    Sets the \a urls that are located in the sidebar.

    For instance:

    \snippet filedialogurls.cpp 0

    The file dialog will then look like this:

    \image filedialogurls.png

    \sa sidebarUrls()
*/
void RemoteFileDialog::setSidebarUrls(const QList<QUrl> &urls)
{
    Q_D(RemoteFileDialog);
    auto u = urls;
    u.prepend(QUrl(QLatin1String("home:")));
    u.prepend(QUrl(QLatin1String("file:")));
    d->qFileDialogUi->sidebar->setUrls(u);
}

/*!
    \since 4.3
    Returns a list of urls that are currently in the sidebar
*/
QList<QUrl> RemoteFileDialog::sidebarUrls() const
{
    Q_D(const RemoteFileDialog);
    return d->qFileDialogUi->sidebar->urls();
}

/*!
    \reimp
*/
void RemoteFileDialog::changeEvent(QEvent *e)
{
    Q_D(RemoteFileDialog);
    if (e->type() == QEvent::LanguageChange) {
        d->retranslateWindowTitle();
        d->retranslateStrings();
    }
    QDialog::changeEvent(e);
}

RemoteFileDialogPrivate::RemoteFileDialogPrivate(RemoteFileDialog *qq)
: q_ptr(qq)
,
#ifndef QT_NO_PROXYMODEL
proxyModel(0)
,
#endif
model(0)
, currentHistoryLocation(-1)
, showHiddenAction(0)
, useDefaultCaption(true)
, qFileDialogUi(0)
, options(RemoteFileDialogOptions::create())
{}

RemoteFileDialogPrivate::~RemoteFileDialogPrivate()
{}

void RemoteFileDialogPrivate::updateCursor(bool waiting)
{
    Q_Q(RemoteFileDialog);
    if (waiting) {
        qFileDialogUi->listView->setCursor(Qt::WaitCursor);
        qFileDialogUi->treeView->setCursor(Qt::WaitCursor);
    } else {
        qFileDialogUi->listView->unsetCursor();
        qFileDialogUi->treeView->unsetCursor();
    }
}

void RemoteFileDialogPrivate::retranslateWindowTitle()
{
    Q_Q(RemoteFileDialog);
    if (!useDefaultCaption || setWindowTitle != q->windowTitle())
        return;
    if (q->acceptMode() == RemoteFileDialog::AcceptOpen) {
        const RemoteFileDialog::FileMode fileMode = q->fileMode();
        if (fileMode == RemoteFileDialog::Directory)
            q->setWindowTitle(RemoteFileDialog::tr("Find Directory"));
        else
            q->setWindowTitle(RemoteFileDialog::tr("Open"));
    } else
        q->setWindowTitle(RemoteFileDialog::tr("Save As"));

    setWindowTitle = q->windowTitle();
}

void RemoteFileDialogPrivate::setLastVisitedDirectory(const QUrl &dir)
{
    *lastVisitedDir() = dir;
}

void RemoteFileDialogPrivate::updateLookInLabel()
{
    if (options->isLabelExplicitlySet(RemoteFileDialogOptions::LookIn))
        setLabelTextControl(RemoteFileDialog::LookIn, options->labelText(RemoteFileDialogOptions::LookIn));
}

void RemoteFileDialogPrivate::updateFileNameLabel()
{
    if (options->isLabelExplicitlySet(RemoteFileDialogOptions::FileName)) {
        setLabelTextControl(RemoteFileDialog::FileName, options->labelText(RemoteFileDialogOptions::FileName));
    } else {
        switch (q_func()->fileMode()) {
        case RemoteFileDialog::Directory:
            setLabelTextControl(RemoteFileDialog::FileName, RemoteFileDialog::tr("Directory:"));
            break;
        default:
            setLabelTextControl(RemoteFileDialog::FileName, RemoteFileDialog::tr("File &name:"));
            break;
        }
    }
}

void RemoteFileDialogPrivate::updateFileTypeLabel()
{
    if (options->isLabelExplicitlySet(RemoteFileDialogOptions::FileType))
        setLabelTextControl(RemoteFileDialog::FileType, options->labelText(RemoteFileDialogOptions::FileType));
}

void RemoteFileDialogPrivate::updateOkButtonText(bool saveAsOnFolder)
{
    Q_Q(RemoteFileDialog);
    // 'Save as' at a folder: Temporarily change to "Open".
    if (saveAsOnFolder) {
        setLabelTextControl(RemoteFileDialog::Accept, RemoteFileDialog::tr("&Open"));
    } else if (options->isLabelExplicitlySet(RemoteFileDialogOptions::Accept)) {
        setLabelTextControl(RemoteFileDialog::Accept, options->labelText(RemoteFileDialogOptions::Accept));
        return;
    } else {
        switch (q->fileMode()) {
        case RemoteFileDialog::Directory:
            setLabelTextControl(RemoteFileDialog::Accept, RemoteFileDialog::tr("&Choose"));
            break;
        default:
            setLabelTextControl(RemoteFileDialog::Accept, q->acceptMode() == RemoteFileDialog::AcceptOpen
                                                              ? RemoteFileDialog::tr("&Open")
                                                              : RemoteFileDialog::tr("&Save"));
            break;
        }
    }
}

void RemoteFileDialogPrivate::updateCancelButtonText()
{
    if (options->isLabelExplicitlySet(RemoteFileDialogOptions::Reject))
        setLabelTextControl(RemoteFileDialog::Reject, options->labelText(RemoteFileDialogOptions::Reject));
}

void RemoteFileDialogPrivate::retranslateStrings()
{
    Q_Q(RemoteFileDialog);
    /* WIDGETS */
    if (options->useDefaultNameFilters())
        q->setNameFilter(RemoteFileDialogOptions::defaultNameFilterString());

    QList<QAction *> actions = qFileDialogUi->treeView->header()->actions();
    QAbstractItemModel *abstractModel = model;
#ifndef QT_NO_PROXYMODEL
    if (proxyModel)
        abstractModel = proxyModel;
#endif
    int total = qMin(abstractModel->columnCount(QModelIndex()), actions.count() + 1);
    for (int i = 1; i < total; ++i) {
        actions.at(i - 1)->setText(RemoteFileDialog::tr("Show ") +
                                   abstractModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
    }

    /* MENU ACTIONS */
    showHiddenAction->setText(RemoteFileDialog::tr("Show &hidden files"));
    newFolderAction->setText(RemoteFileDialog::tr("&New Folder"));
    qFileDialogUi->retranslateUi(q);
    updateLookInLabel();
    updateFileNameLabel();
    updateFileTypeLabel();
    updateCancelButtonText();
}

void RemoteFileDialogPrivate::emitFilesSelected(const QStringList &files)
{
    Q_Q(RemoteFileDialog);
    emit q->filesSelected(files);
    if (files.count() == 1)
        emit q->fileSelected(files.first());
}

bool RemoteFileDialogPrivate::usingWidgets() const
{
    //return !nativeDialogInUse && qFileDialogUi;
    return qFileDialogUi;
}

/*!
    \since 4.5
    Sets the given \a option to be enabled if \a on is true; otherwise,
    clears the given \a option.

    \sa options, testOption()
*/
void RemoteFileDialog::setOption(Option option, bool on)
{
    const RemoteFileDialog::Options previousOptions = options();
    if (!(previousOptions & option) != !on)
        setOptions(previousOptions ^ option);
}

/*!
    \since 4.5

    Returns \c true if the given \a option is enabled; otherwise, returns
    false.

    \sa options, setOption()
*/
bool RemoteFileDialog::testOption(Option option) const
{
    Q_D(const RemoteFileDialog);
    return d->options->testOption(static_cast<RemoteFileDialogOptions::FileDialogOption>(option));
}

/*!
    \property RemoteFileDialog::options
    \brief the various options that affect the look and feel of the dialog
    \since 4.5

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the
    dialog is visible is not guaranteed to have an immediate effect on the
    dialog (depending on the option and on the platform).

    \sa setOption(), testOption()
*/
void RemoteFileDialog::setOptions(Options options)
{
    Q_D(RemoteFileDialog);

    Options changed = (options ^ RemoteFileDialog::options());
    if (!changed)
        return;

    d->options->setOptions(RemoteFileDialogOptions::FileDialogOptions(int(options)));

    if (d->usingWidgets()) {
        if (changed & DontResolveSymlinks)
            d->model->setResolveSymlinks(!(options & DontResolveSymlinks));
        if (changed & ReadOnly) {
            bool ro = (options & ReadOnly);
            d->model->setReadOnly(ro);
            d->qFileDialogUi->newFolderButton->setEnabled(!ro);
        }
    }

    if (changed & HideNameFilterDetails)
        setNameFilters(d->options->nameFilters());

    if (changed & ShowDirsOnly)
        setFilter((options & ShowDirsOnly) ? filter() & ~QDir::Files : filter() | QDir::Files);
}

RemoteFileDialog::Options RemoteFileDialog::options() const
{
    Q_D(const RemoteFileDialog);
    return RemoteFileDialog::Options(int(d->options->options()));
}

/*!
    \overload

    \since 4.5

    This function connects one of its signals to the slot specified by \a receiver
    and \a member. The specific signal depends is filesSelected() if fileMode is
    ExistingFiles and fileSelected() if fileMode is anything else.

    The signal will be disconnected from the slot when the dialog is closed.
*/
void RemoteFileDialog::open(QObject *receiver, const char *member)
{
    Q_D(RemoteFileDialog);
    const char *signal =
        (fileMode() == ExistingFiles) ? SIGNAL(filesSelected(QStringList)) : SIGNAL(fileSelected(QString));
    connect(this, signal, receiver, member);
    d->signalToDisconnectOnClose = signal;
    d->receiverToDisconnectOnClose = receiver;
    d->memberToDisconnectOnClose = member;

    QDialog::open();
}


/*!
    \reimp
*/
void RemoteFileDialog::setVisible(bool visible)
{
    Q_D(RemoteFileDialog);
    if (isVisible() && !visible)
        d->saveSettings();

    if (visible) {
        if (testAttribute(Qt::WA_WState_ExplicitShowHide) && !testAttribute(Qt::WA_WState_Hidden))
            return;
    } else if (testAttribute(Qt::WA_WState_ExplicitShowHide) && testAttribute(Qt::WA_WState_Hidden))
        return;

    if (visible && d->usingWidgets())
        d->qFileDialogUi->fileNameEdit->setFocus();

    QDialog::setVisible(visible);
}

/*!
    \internal
    set the directory to url
*/
void RemoteFileDialogPrivate::_q_goToUrl(const QUrl &url)
{
#if 0
    //The shortcut in the side bar may have a parent that is not fetched yet (e.g. an hidden file)
    //so we force the fetching
    RemoteFileSystemModelPrivate::FileSystemNode *node = model->d_func()->node(url.toLocalFile(), true);
    QModelIndex idx =  model->index(node);
    _q_enterDirectory(idx);
#else
    auto path = url.toLocalFile();
    if (url.scheme() == QLatin1String("home"))
        path = model->homePath().toString();
    QModelIndex idx = model->fsIndex(path);
    model->fetchMore(idx);
    _q_enterDirectory(idx);
#endif
}

/*!
    \fn void RemoteFileDialog::setDirectory(const QDir &directory)

    \overload
*/

/*!
    Sets the file dialog's current \a directory.

    \note On iOS, if you set \a directory to \l{QStandardPaths::standardLocations()}
        {QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).last()},
        a native image picker dialog will be used for accessing the user's photo album.
        The filename returned can be loaded using QFile and related APIs.
        For this to be enabled, the Info.plist assigned to QMAKE_INFO_PLIST in the
        project file must contain the key \c NSPhotoLibraryUsageDescription. See
        Info.plist documentation from Apple for more information regarding this key.
        This feature was added in Qt 5.5.
*/
void RemoteFileDialog::setDirectory(const QString &directory)
{
    Q_D(RemoteFileDialog);
    QString newDirectory = directory;
    //we remove .. and . from the given path if exist
    if (!directory.isEmpty()) {
        newDirectory = QDir::cleanPath(directory);
        if (QDir::isRelativePath(newDirectory)) {
            newDirectory = d->model->workingDirectory() + "/" + newDirectory;
        }
    }

    if (!directory.isEmpty() && newDirectory.isEmpty())
        return;

    QUrl newDirUrl = QUrl::fromLocalFile(newDirectory);
    RemoteFileDialogPrivate::setLastVisitedDirectory(newDirUrl);

    d->options->setInitialDirectory(QUrl::fromLocalFile(directory));
    if (d->rootPath() == newDirectory)
        return;
    QModelIndex root = d->model->setRootPath(newDirectory);
    d->qFileDialogUi->newFolderButton->setEnabled(d->model->flags(root) & Qt::ItemIsDropEnabled);
    if (root != d->rootIndex()) {
        if (directory.endsWith(QLatin1Char('/')))
            d->completer->setCompletionPrefix(newDirectory);
        else
            d->completer->setCompletionPrefix(newDirectory + QLatin1Char('/'));
        d->setRootIndex(root);
    }
    d->qFileDialogUi->listView->selectionModel()->clear();
}

/*!
    Returns the directory currently being displayed in the dialog.
*/
QString RemoteFileDialog::directory() const
{
    Q_D(const RemoteFileDialog);
    return QDir::cleanPath(d->rootPath());
}

/*!
    Sets the file dialog's current \a directory url.

    \note The non-native RemoteFileDialog supports only local files.

    \note On Windows, it is possible to pass URLs representing
    one of the \e {virtual folders}, such as "Computer" or "Network".
    This is done by passing a QUrl using the scheme \c clsid followed
    by the CLSID value with the curly braces removed. For example the URL
    \c clsid:374DE290-123F-4565-9164-39C4925E467B denotes the download
    location. For a complete list of possible values, see the MSDN documentation on
    \l{https://msdn.microsoft.com/en-us/library/windows/desktop/dd378457.aspx}{KNOWNFOLDERID}.
    This feature was added in Qt 5.5.

    \sa QUuid
    \since 5.2
*/
void RemoteFileDialog::setDirectoryUrl(const QUrl &directory)
{
    Q_D(RemoteFileDialog);
    if (!directory.isValid())
        return;

    RemoteFileDialogPrivate::setLastVisitedDirectory(directory);
    d->options->setInitialDirectory(directory);

    if (directory.scheme() == QLatin1String("home"))
        setDirectory(d->model->homePath().toString());
    else if (directory.isLocalFile())
        setDirectory(directory.toLocalFile());
    else if (Q_UNLIKELY(d->usingWidgets()))
        qWarning("Non-native RemoteFileDialog supports only local files");
}

/*!
    Returns the url of the directory currently being displayed in the dialog.

    \since 5.2
*/
QUrl RemoteFileDialog::directoryUrl() const
{
    Q_D(const RemoteFileDialog);
    return QUrl::fromLocalFile(directory());
}

// FIXME Qt 5.4: Use upcoming QVolumeInfo class to determine this information?
static inline bool isCaseSensitiveFileSystem(const QString &path)
{
    Q_UNUSED(path)
#if defined(Q_OS_WIN)
    // Return case insensitive unconditionally, even if someone has a case sensitive
    // file system mounted, wrongly capitalized drive letters will cause mismatches.
    return false;
#elif defined(Q_OS_OSX)
    return pathconf(QFile::encodeName(path).constData(), _PC_CASE_SENSITIVE);
#else
    return true;
#endif
}

// Determine the file name to be set on the line edit from the path
// passed to selectFile() in mode RemoteFileDialog::AcceptSave.
QString RemoteFileDialog::fileFromPath(const QString &rootPath, QString path)
{
    if (!QDir::isRelativePath(path))
        return path;
    if (path.startsWith(rootPath, isCaseSensitiveFileSystem(rootPath) ? Qt::CaseSensitive : Qt::CaseInsensitive))
        path.remove(0, rootPath.size());

    if (path.isEmpty())
        return path;

    if (path.at(0) == QDir::separator()
        //On Windows both cases can happen
        || (m_model->isWindows() && path.at(0) == QLatin1Char('/'))) {
        path.remove(0, 1);
    }
    return path;
}

/*!
    Selects the given \a filename in the file dialog.

    \sa selectedFiles()
*/
void RemoteFileDialog::selectFile(const QString &filename)
{
    Q_D(RemoteFileDialog);
    if (filename.isEmpty())
        return;

    d->qFileDialogUi->listView->selectionModel()->clear();
    auto fn = filename;
    QModelIndex index = d->model->fsIndex(fn);
    if (index.isValid()) {
        if (!QDir::isRelativePath(fn)) {
            if (index.isValid() && index.parent() != d->rootIndex())
                setDirectory(d->model->filePath(index.parent()));
        }
        d->select(index);
    } else {
        auto pathname = QDir::cleanPath(fn);
        auto comps = pathname.split("/");
        fn = comps.back();
        comps.pop_back();
        auto dir = comps.join("/");
        setDirectory(dir);
    }

    if (!isVisible() || !d->lineEdit()->hasFocus())
        d->lineEdit()->setText(index.isValid() ? index.data().toString() : fileFromPath(d->rootPath(), fn));
}

/*!
    Selects the given \a url in the file dialog.

    \note The non-native RemoteFileDialog supports only local files.

    \sa selectedUrls()
    \since 5.2
*/
void RemoteFileDialog::selectUrl(const QUrl &url)
{
    Q_D(RemoteFileDialog);
    if (!url.isValid())
        return;

    if (url.scheme() == "home") {
        selectFile(d->model->homePath().toString());
    } else if (url.isLocalFile())
        selectFile(url.toLocalFile());
    else
        qWarning("Non-native RemoteFileDialog supports only local files");
}

Q_AUTOTEST_EXPORT QString qt_tildeExpansion(AbstractFileSystemModel *model, const QString &path)
{
    if (!path.startsWith(QLatin1Char('~')))
        return path;
    int separatorPosition = path.indexOf(QDir::separator());
    if (separatorPosition < 0)
        separatorPosition = path.size();
    if (separatorPosition == 1) {
        return model->homePath().toString() + path.midRef(1);
    } else {
        return path;
    }
}


/**
    Returns the text in the line edit which can be one or more file names
  */
QStringList RemoteFileDialogPrivate::typedFiles() const
{
    Q_Q(const RemoteFileDialog);
    QStringList files;
    QString editText = lineEdit()->text();
    if (!editText.contains(QLatin1Char('"'))) {
        if (model->isWindows()) {
            files << editText;
        } else {
            const QString prefix = q->directory() + "/";
            if (QFile::exists(prefix + editText))
                files << editText;
#ifndef Q_OS_WIN
            else
                files << qt_tildeExpansion(model, editText);
#endif
        }
    } else {
        // " is used to separate files like so: "file1" "file2" "file3" ...
        // ### need escape character for filenames with quotes (")
        QStringList tokens = editText.split(QLatin1Char('\"'));
        for (int i = 0; i < tokens.size(); ++i) {
            if ((i % 2) == 0)
                continue; // Every even token is a separator
            if (model->isWindows()) {
                files << toInternal(tokens.at(i));
            } else {
                const QString token = tokens.at(i);
                const QString prefix = q->directory() + "/";
                if (QFile::exists(prefix + token))
                    files << token;
#ifndef Q_OS_WIN
                else
                    files << qt_tildeExpansion(model, token);
#endif
            }
        }
    }
    return addDefaultSuffixToFiles(files);
}

// Return selected files without defaulting to the root of the file system model
// used for initializing RemoteFileDialogOptions for native dialogs. The default is
// not suitable for native dialogs since it mostly equals directory().
QList<QUrl> RemoteFileDialogPrivate::userSelectedFiles() const
{
    QList<QUrl> files;

    const QModelIndexList selectedRows = qFileDialogUi->listView->selectionModel()->selectedRows();
    files.reserve(selectedRows.size());
    for (const QModelIndex &index: selectedRows)
        files.append(QUrl::fromLocalFile(index.data(AbstractFileSystemModel::FilePathRole).toString()));

    if (files.isEmpty() && !lineEdit()->text().isEmpty()) {
        const QStringList typedFilesList = typedFiles();
        files.reserve(typedFilesList.size());
        for (const QString &path: typedFilesList)
            files.append(QUrl::fromLocalFile(path));
    }

    return files;
}

QStringList RemoteFileDialogPrivate::addDefaultSuffixToFiles(const QStringList &filesToFix) const
{
    QStringList files;
    for (int i = 0; i < filesToFix.size(); ++i) {
        QString name = toInternal(filesToFix.at(i));
        QFileInfo info(name);
        // if the filename has no suffix, add the default suffix
        const QString defaultSuffix = options->defaultSuffix();
        if (!defaultSuffix.isEmpty() && !info.isDir() && name.lastIndexOf(QLatin1Char('.')) == -1)
            name += QLatin1Char('.') + defaultSuffix;
        if (info.isAbsolute()) {
            files.append(name);
        } else {
            // at this point the path should only have Qt path separators.
            // This check is needed since we might be at the root directory
            // and on Windows it already ends with slash.
            QString path = rootPath();
            if (!path.endsWith(QLatin1Char('/')))
                path += QLatin1Char('/');
            path += name;
            files.append(path);
        }
    }
    return files;
}

QList<QUrl> RemoteFileDialogPrivate::addDefaultSuffixToUrls(const QList<QUrl> &urlsToFix) const
{
    QList<QUrl> urls;
    const int numUrlsToFix = urlsToFix.size();
    urls.reserve(numUrlsToFix);
    for (int i = 0; i < numUrlsToFix; ++i) {
        QUrl url = urlsToFix.at(i);
        // if the filename has no suffix, add the default suffix
        const QString defaultSuffix = options->defaultSuffix();
        if (!defaultSuffix.isEmpty() && !url.path().endsWith(QLatin1Char('/')) &&
            url.path().lastIndexOf(QLatin1Char('.')) == -1)
            url.setPath(url.path() + QLatin1Char('.') + defaultSuffix);
        urls.append(url);
    }
    return urls;
}


/*!
    Returns a list of strings containing the absolute paths of the
    selected files in the dialog. If no files are selected, or
    the mode is not ExistingFiles or ExistingFile, selectedFiles() contains the current path in the viewport.

    \sa selectedNameFilter(), selectFile()
*/
QStringList RemoteFileDialog::selectedFiles() const
{
    Q_D(const RemoteFileDialog);

    QStringList files;
    const QList<QUrl> userSelectedFiles = d->userSelectedFiles();
    files.reserve(userSelectedFiles.size());
    for (const QUrl &file: userSelectedFiles)
        files.append(file.toLocalFile());
    if (files.isEmpty() && d->usingWidgets()) {
        const FileMode fm = fileMode();
        if (fm != ExistingFile && fm != ExistingFiles)
            files.append(d->rootIndex().data(AbstractFileSystemModel::FilePathRole).toString());
    }
    return files;
}

/*!
    Returns a list of urls containing the selected files in the dialog.
    If no files are selected, or the mode is not ExistingFiles or
    ExistingFile, selectedUrls() contains the current path in the viewport.

    \sa selectedNameFilter(), selectUrl()
    \since 5.2
*/
QList<QUrl> RemoteFileDialog::selectedUrls() const
{
    Q_D(const RemoteFileDialog);
    QList<QUrl> urls;
    const QStringList selectedFileList = selectedFiles();
    urls.reserve(selectedFileList.size());
    for (const QString &file: selectedFileList)
        urls.append(QUrl::fromLocalFile(file));
    return urls;
}

/*
    Makes a list of filters from ;;-separated text.
    Used by the mac and windows implementations
*/
QStringList rfb_make_filter_list(const QString &filter)
{
    QString f(filter);

    if (f.isEmpty())
        return QStringList();

    QString sep(QLatin1String(";;"));
    int i = f.indexOf(sep, 0);
    if (i == -1) {
        if (f.indexOf(QLatin1Char('\n'), 0) != -1) {
            sep = QLatin1Char('\n');
            i = f.indexOf(sep, 0);
        }
    }

    return f.split(sep);
}

/*!
    \since 4.4

    Sets the filter used in the file dialog to the given \a filter.

    If \a filter contains a pair of parentheses containing one or more
    filename-wildcard patterns, separated by spaces, then only the
    text contained in the parentheses is used as the filter. This means
    that these calls are all equivalent:

    \snippet code/src_gui_dialogs_qfiledialog.cpp 6

    \sa setMimeTypeFilters(), setNameFilters()
*/
void RemoteFileDialog::setNameFilter(const QString &filter)
{
    setNameFilters(rfb_make_filter_list(filter));
}


/*!
    \property RemoteFileDialog::nameFilterDetailsVisible
    \obsolete
    \brief This property holds whether the filter details is shown or not.
    \since 4.4

    When this property is \c true (the default), the filter details are shown
    in the combo box.  When the property is set to false, these are hidden.

    Use setOption(HideNameFilterDetails, !\e enabled) or
    !testOption(HideNameFilterDetails).
*/
void RemoteFileDialog::setNameFilterDetailsVisible(bool enabled)
{
    setOption(HideNameFilterDetails, !enabled);
}

bool RemoteFileDialog::isNameFilterDetailsVisible() const
{
    return !testOption(HideNameFilterDetails);
}


/*
    Strip the filters by removing the details, e.g. (*.*).
*/
QStringList rfb_strip_filters(const QStringList &filters)
{
    QStringList strippedFilters;
    QRegExp r(QString::fromLatin1(remoteFilterRegExp));
    const int numFilters = filters.count();
    strippedFilters.reserve(numFilters);
    for (int i = 0; i < numFilters; ++i) {
        QString filterName;
        int index = r.indexIn(filters[i]);
        if (index >= 0)
            filterName = r.cap(1);
        strippedFilters.append(filterName.simplified());
    }
    return strippedFilters;
}


/*!
    \since 4.4

    Sets the \a filters used in the file dialog.

    Note that the filter \b{*.*} is not portable, because the historical
    assumption that the file extension determines the file type is not
    consistent on every operating system. It is possible to have a file with no
    dot in its name (for example, \c Makefile). In a native Windows file
    dialog, \b{*.*} will match such files, while in other types of file dialogs
    it may not. So it is better to use \b{*} if you mean to select any file.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 7

    \l setMimeTypeFilters() has the advantage of providing all possible name
    filters for each file type. For example, JPEG images have three possible
    extensions; if your application can open such files, selecting the
    \c image/jpeg mime type as a filter will allow you to open all of them.
*/
void RemoteFileDialog::setNameFilters(const QStringList &filters)
{
    Q_D(RemoteFileDialog);
    QStringList cleanedFilters;
    const int numFilters = filters.count();
    cleanedFilters.reserve(numFilters);
    for (int i = 0; i < numFilters; ++i) {
        cleanedFilters << filters[i].simplified();
    }
    d->options->setNameFilters(cleanedFilters);

    d->qFileDialogUi->fileTypeCombo->clear();
    if (cleanedFilters.isEmpty())
        return;

    if (testOption(HideNameFilterDetails))
        d->qFileDialogUi->fileTypeCombo->addItems(rfb_strip_filters(cleanedFilters));
    else
        d->qFileDialogUi->fileTypeCombo->addItems(cleanedFilters);

    d->_q_useNameFilter(0);
}

/*!
    \since 4.4

    Returns the file type filters that are in operation on this file
    dialog.
*/
QStringList RemoteFileDialog::nameFilters() const
{
    return d_func()->options->nameFilters();
}

/*!
    \since 4.4

    Sets the current file type \a filter. Multiple filters can be
    passed in \a filter by separating them with semicolons or spaces.

    \sa setNameFilter(), setNameFilters(), selectedNameFilter()
*/
void RemoteFileDialog::selectNameFilter(const QString &filter)
{
    Q_D(RemoteFileDialog);
    d->options->setInitiallySelectedNameFilter(filter);
    int i = -1;
    if (testOption(HideNameFilterDetails)) {
        const QStringList filters = rfb_strip_filters(rfb_make_filter_list(filter));
        if (!filters.isEmpty())
            i = d->qFileDialogUi->fileTypeCombo->findText(filters.first());
    } else {
        i = d->qFileDialogUi->fileTypeCombo->findText(filter);
    }
    if (i >= 0) {
        d->qFileDialogUi->fileTypeCombo->setCurrentIndex(i);
        d->_q_useNameFilter(d->qFileDialogUi->fileTypeCombo->currentIndex());
    }
}

/*!
 * \since 5.9
 * \return The mimetype of the file that the user selected in the file dialog.
 */
QString RemoteFileDialog::selectedMimeTypeFilter() const
{
    Q_D(const RemoteFileDialog);

    return d->options->initiallySelectedMimeTypeFilter();
}

/*!
    \since 4.4

    Returns the filter that the user selected in the file dialog.

    \sa selectedFiles()
*/
QString RemoteFileDialog::selectedNameFilter() const
{
    Q_D(const RemoteFileDialog);

    return d->qFileDialogUi->fileTypeCombo->currentText();
}

/*!
    \since 4.4

    Returns the filter that is used when displaying files.

    \sa setFilter()
*/
QDir::Filters RemoteFileDialog::filter() const
{
    Q_D(const RemoteFileDialog);
    return d->model->filter();
}

/*!
    \since 4.4

    Sets the filter used by the model to \a filters. The filter is used
    to specify the kind of files that should be shown.

    \sa filter()
*/

void RemoteFileDialog::setFilter(QDir::Filters filters)
{
    Q_D(RemoteFileDialog);
    d->options->setFilter(filters);

    d->model->setFilter(filters);
    d->showHiddenAction->setChecked((filters & QDir::Hidden));
}

#ifndef QT_NO_MIMETYPE

static QString nameFilterForMime(const QString &mimeType)
{
    QMimeDatabase db;
    QMimeType mime(db.mimeTypeForName(mimeType));
    if (mime.isValid()) {
        if (mime.isDefault()) {
            return RemoteFileDialog::tr("All files (*)");
        } else {
            const QString patterns = mime.globPatterns().join(QLatin1Char(' '));
            return mime.comment() + QLatin1String(" (") + patterns + QLatin1Char(')');
        }
    }
    return QString();
}

/*!
    \since 5.2

    Sets the \a filters used in the file dialog, from a list of MIME types.

    Convenience method for setNameFilters().
    Uses QMimeType to create a name filter from the glob patterns and description
    defined in each MIME type.

    Use application/octet-stream for the "All files (*)" filter, since that
    is the base MIME type for all files.

    Calling setMimeTypeFilters overrides any previously set name filters,
    and changes the return value of nameFilters().

    \snippet code/src_gui_dialogs_qfiledialog.cpp 13
*/
void RemoteFileDialog::setMimeTypeFilters(const QStringList &filters)
{
    Q_D(RemoteFileDialog);
    QStringList nameFilters;
    for (const QString &mimeType: filters) {
        const QString text = nameFilterForMime(mimeType);
        if (!text.isEmpty())
            nameFilters.append(text);
    }
    setNameFilters(nameFilters);
    d->options->setMimeTypeFilters(filters);
}

/*!
    \since 5.2

    Returns the MIME type filters that are in operation on this file
    dialog.
*/
QStringList RemoteFileDialog::mimeTypeFilters() const
{
    return d_func()->options->mimeTypeFilters();
}

/*!
    \since 5.2

    Sets the current MIME type \a filter.

*/
void RemoteFileDialog::selectMimeTypeFilter(const QString &filter)
{
    Q_D(RemoteFileDialog);
    d->options->setInitiallySelectedMimeTypeFilter(filter);

    const QString filterForMime = nameFilterForMime(filter);

    selectNameFilter(filterForMime);
}

#endif // QT_NO_MIMETYPE

/*!
    \property RemoteFileDialog::viewMode
    \brief the way files and directories are displayed in the dialog

    By default, the \c Detail mode is used to display information about
    files and directories.

    \sa ViewMode
*/
void RemoteFileDialog::setViewMode(RemoteFileDialog::ViewMode mode)
{
    Q_D(RemoteFileDialog);
    d->options->setViewMode(static_cast<RemoteFileDialogOptions::ViewMode>(mode));
    if (mode == Detail)
        d->_q_showDetailsView();
    else
        d->_q_showListView();
}

RemoteFileDialog::ViewMode RemoteFileDialog::viewMode() const
{
    Q_D(const RemoteFileDialog);
    return (d->qFileDialogUi->stackedWidget->currentWidget() == d->qFileDialogUi->listView->parent()
                ? RemoteFileDialog::List
                : RemoteFileDialog::Detail);
}

/*!
    \property RemoteFileDialog::fileMode
    \brief the file mode of the dialog

    The file mode defines the number and type of items that the user is
    expected to select in the dialog.

    By default, this property is set to AnyFile.

    This function will set the labels for the FileName and
    \l{RemoteFileDialog::}{Accept} \l{DialogLabel}s. It is possible to set
    custom text after the call to setFileMode().

    \sa FileMode
*/
void RemoteFileDialog::setFileMode(RemoteFileDialog::FileMode mode)
{
    Q_D(RemoteFileDialog);
    d->options->setFileMode(static_cast<RemoteFileDialogOptions::FileMode>(mode));

    d->retranslateWindowTitle();

    // set selection mode and behavior
    QAbstractItemView::SelectionMode selectionMode;
    if (mode == RemoteFileDialog::ExistingFiles)
        selectionMode = QAbstractItemView::ExtendedSelection;
    else
        selectionMode = QAbstractItemView::SingleSelection;
    d->qFileDialogUi->listView->setSelectionMode(selectionMode);
    d->qFileDialogUi->treeView->setSelectionMode(selectionMode);
    // set filter
    d->model->setFilter(d->filterForMode(filter()));
    // setup file type for directory
    if (mode == Directory) {
        d->qFileDialogUi->fileTypeCombo->clear();
        d->qFileDialogUi->fileTypeCombo->addItem(tr("Directories"));
        d->qFileDialogUi->fileTypeCombo->setEnabled(false);
    }
    d->updateFileNameLabel();
    d->updateOkButtonText();
    d->qFileDialogUi->fileTypeCombo->setEnabled(!testOption(ShowDirsOnly));
    d->_q_updateOkButton();
}

RemoteFileDialog::FileMode RemoteFileDialog::fileMode() const
{
    Q_D(const RemoteFileDialog);
    return static_cast<FileMode>(d->options->fileMode());
}

/*!
    \property RemoteFileDialog::acceptMode
    \brief the accept mode of the dialog

    The action mode defines whether the dialog is for opening or saving files.

    By default, this property is set to \l{AcceptOpen}.

    \sa AcceptMode
*/
void RemoteFileDialog::setAcceptMode(RemoteFileDialog::AcceptMode mode)
{
    Q_D(RemoteFileDialog);
    d->options->setAcceptMode(static_cast<RemoteFileDialogOptions::AcceptMode>(mode));
    // clear WA_DontShowOnScreen so that d->canBeNativeDialog() doesn't return false incorrectly
    setAttribute(Qt::WA_DontShowOnScreen, false);
    QDialogButtonBox::StandardButton button = (mode == AcceptOpen ? QDialogButtonBox::Open : QDialogButtonBox::Save);
    d->qFileDialogUi->buttonBox->setStandardButtons(button | QDialogButtonBox::Cancel);
    d->qFileDialogUi->buttonBox->button(button)->setEnabled(false);
    d->_q_updateOkButton();
    if (mode == AcceptSave) {
        d->qFileDialogUi->lookInCombo->setEditable(false);
    }
    d->retranslateWindowTitle();
}

/*!
    \property RemoteFileDialog::supportedSchemes
    \brief the URL schemes that the file dialog should allow navigating to.
    \since 5.6

    Setting this property allows to restrict the type of URLs the
    user will be able to select. It is a way for the application to declare
    the protocols it will support to fetch the file content. An empty list
    means that no restriction is applied (the default).
    Supported for local files ("file" scheme) is implicit and always enabled;
    it is not necessary to include it in the restriction.
*/

void RemoteFileDialog::setSupportedSchemes(const QStringList &schemes)
{
    Q_D(RemoteFileDialog);
    d->options->setSupportedSchemes(schemes);
}

QStringList RemoteFileDialog::supportedSchemes() const
{
    return d_func()->options->supportedSchemes();
}

/*
    Returns the file system model index that is the root index in the
    views
*/
QModelIndex RemoteFileDialogPrivate::rootIndex() const
{
    return mapToSource(qFileDialogUi->listView->rootIndex());
}

QAbstractItemView *RemoteFileDialogPrivate::currentView() const
{
    if (!qFileDialogUi->stackedWidget)
        return 0;
    if (qFileDialogUi->stackedWidget->currentWidget() == qFileDialogUi->listView->parent())
        return qFileDialogUi->listView;
    return qFileDialogUi->treeView;
}

QLineEdit *RemoteFileDialogPrivate::lineEdit() const
{
    return (QLineEdit *)qFileDialogUi->fileNameEdit;
}

int RemoteFileDialogPrivate::maxNameLength(const QString &path)
{
#if defined(Q_OS_UNIX)
    return ::pathconf(QFile::encodeName(path).data(), _PC_NAME_MAX);
#elif defined(Q_OS_WINRT)
    Q_UNUSED(path);
    return MAX_PATH;
#elif defined(Q_OS_WIN)
    DWORD maxLength;
    const QString drive = path.left(3);
    if (::GetVolumeInformation(/*reinterpret_cast<const wchar_t *>*/ (drive.toLatin1()), NULL, 0, NULL, &maxLength,
                               NULL, NULL, 0) == false)
        return -1;
    return maxLength;
#else
    Q_UNUSED(path);
#endif
    return -1;
}

/*
    Sets the view root index to be the file system model index
*/
void RemoteFileDialogPrivate::setRootIndex(const QModelIndex &index) const
{
    Q_ASSERT(index.isValid() ? index.model() == model : true);
    QModelIndex idx = mapFromSource(index);
    qFileDialogUi->treeView->setRootIndex(idx);
    qFileDialogUi->listView->setRootIndex(idx);
}
/*
    Select a file system model index
    returns the index that was selected (or not depending upon sortfilterproxymodel)
*/
QModelIndex RemoteFileDialogPrivate::select(const QModelIndex &index) const
{
    Q_ASSERT(index.isValid() ? index.model() == model : true);

    QModelIndex idx = mapFromSource(index);
    if (idx.isValid() && !qFileDialogUi->listView->selectionModel()->isSelected(idx))
        qFileDialogUi->listView->selectionModel()->select(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    return idx;
}

RemoteFileDialog::AcceptMode RemoteFileDialog::acceptMode() const
{
    Q_D(const RemoteFileDialog);
    return static_cast<AcceptMode>(d->options->acceptMode());
}

/*!
    \property RemoteFileDialog::readOnly
    \obsolete
    \brief Whether the filedialog is read-only

    If this property is set to false, the file dialog will allow renaming,
    and deleting of files and directories and creating directories.

    Use setOption(ReadOnly, \e enabled) or testOption(ReadOnly) instead.
*/
void RemoteFileDialog::setReadOnly(bool enabled)
{
    setOption(ReadOnly, enabled);
}

bool RemoteFileDialog::isReadOnly() const
{
    return testOption(ReadOnly);
}

/*!
    \property RemoteFileDialog::resolveSymlinks
    \obsolete
    \brief whether the filedialog should resolve shortcuts

    If this property is set to true, the file dialog will resolve
    shortcuts or symbolic links.

    Use setOption(DontResolveSymlinks, !\a enabled) or
    !testOption(DontResolveSymlinks).
*/
void RemoteFileDialog::setResolveSymlinks(bool enabled)
{
    setOption(DontResolveSymlinks, !enabled);
}

bool RemoteFileDialog::resolveSymlinks() const
{
    return !testOption(DontResolveSymlinks);
}

/*!
    \property RemoteFileDialog::confirmOverwrite
    \obsolete
    \brief whether the filedialog should ask before accepting a selected file,
    when the accept mode is AcceptSave

    Use setOption(DontConfirmOverwrite, !\e enabled) or
    !testOption(DontConfirmOverwrite) instead.
*/
void RemoteFileDialog::setConfirmOverwrite(bool enabled)
{
    setOption(DontConfirmOverwrite, !enabled);
}

bool RemoteFileDialog::confirmOverwrite() const
{
    return !testOption(DontConfirmOverwrite);
}

/*!
    \property RemoteFileDialog::defaultSuffix
    \brief suffix added to the filename if no other suffix was specified

    This property specifies a string that will be added to the
    filename if it has no suffix already. The suffix is typically
    used to indicate the file type (e.g. "txt" indicates a text
    file).

    If the first character is a dot ('.'), it is removed.
*/
void RemoteFileDialog::setDefaultSuffix(const QString &suffix)
{
    Q_D(RemoteFileDialog);
    d->options->setDefaultSuffix(suffix);
}

QString RemoteFileDialog::defaultSuffix() const
{
    Q_D(const RemoteFileDialog);
    return d->options->defaultSuffix();
}

/*!
    Sets the browsing history of the filedialog to contain the given
    \a paths.
*/
void RemoteFileDialog::setHistory(const QStringList &paths)
{
    Q_D(RemoteFileDialog);
    if (d->usingWidgets())
        d->qFileDialogUi->lookInCombo->setHistory(paths);
}

void RemoteFileDialogComboBox::setHistory(const QStringList &paths)
{
    m_history = paths;

    qInfo() << "setHistory" << paths;

#if 0
    populatePopup();
#else
    // Only populate the first item, showPopup will populate the rest if needed
    QList<QUrl> list;
    QModelIndex idx = d_ptr->model->fsIndex(d_ptr->rootPath());
    //On windows the popup display the "C:\", convert to nativeSeparators
    QUrl url =
        QUrl::fromLocalFile(QDir::toNativeSeparators(idx.data(AbstractFileSystemModel::FilePathRole).toString()));
    if (url.isValid())
        list.append(url);
    urlModel->setUrls(list);
#endif
}

/*!
    Returns the browsing history of the filedialog as a list of paths.
*/
QStringList RemoteFileDialog::history() const
{
    Q_D(const RemoteFileDialog);
    QStringList currentHistory = d->qFileDialogUi->lookInCombo->history();
    //On windows the popup display the "C:\", convert to nativeSeparators
    QString newHistory =
        QDir::toNativeSeparators(d->rootIndex().data(AbstractFileSystemModel::FilePathRole).toString());
    if (!currentHistory.contains(newHistory))
        currentHistory << newHistory;
    return currentHistory;
}

/*!
    Sets the item delegate used to render items in the views in the
    file dialog to the given \a delegate.

    \warning You should not share the same instance of a delegate between views.
    Doing so can cause incorrect or unintuitive editing behavior since each
    view connected to a given delegate may receive the \l{QAbstractItemDelegate::}{closeEditor()}
    signal, and attempt to access, modify or close an editor that has already been closed.

    Note that the model used is AbstractFileSystemModel. It has custom item data roles, which is
    described by the \l{AbstractFileSystemModel::}{Roles} enum. You can use a RemoteFileIconProvider if
    you only want custom icons.

    \sa itemDelegate(), setIconProvider(), AbstractFileSystemModel
*/
void RemoteFileDialog::setItemDelegate(QAbstractItemDelegate *delegate)
{
    Q_D(RemoteFileDialog);
    d->qFileDialogUi->listView->setItemDelegate(delegate);
    d->qFileDialogUi->treeView->setItemDelegate(delegate);
}

/*!
  Returns the item delegate used to render the items in the views in the filedialog.
*/
QAbstractItemDelegate *RemoteFileDialog::itemDelegate() const
{
    Q_D(const RemoteFileDialog);
    return d->qFileDialogUi->listView->itemDelegate();
}

/*!
    Sets the icon provider used by the filedialog to the specified \a provider.
*/
void RemoteFileDialog::setIconProvider(RemoteFileIconProvider *provider)
{
    Q_D(RemoteFileDialog);
    d->model->setIconProvider(provider);
    //It forces the refresh of all entries in the side bar, then we can get new icons
    d->qFileDialogUi->sidebar->setUrls(d->qFileDialogUi->sidebar->urls());
}

/*!
    Returns the icon provider used by the filedialog.
*/
RemoteFileIconProvider *RemoteFileDialog::iconProvider() const
{
    Q_D(const RemoteFileDialog);
    if (!d->model)
        return nullptr;
    return d->model->iconProvider();
}

void RemoteFileDialogPrivate::setLabelTextControl(RemoteFileDialog::DialogLabel label, const QString &text)
{
    if (!qFileDialogUi)
        return;
    switch (label) {
    case RemoteFileDialog::LookIn:
        qFileDialogUi->lookInLabel->setText(text);
        break;
    case RemoteFileDialog::FileName:
        qFileDialogUi->fileNameLabel->setText(text);
        break;
    case RemoteFileDialog::FileType:
        qFileDialogUi->fileTypeLabel->setText(text);
        break;
    case RemoteFileDialog::Accept:
        if (q_func()->acceptMode() == RemoteFileDialog::AcceptOpen) {
            if (QPushButton *button = qFileDialogUi->buttonBox->button(QDialogButtonBox::Open))
                button->setText(text);
        } else {
            if (QPushButton *button = qFileDialogUi->buttonBox->button(QDialogButtonBox::Save))
                button->setText(text);
        }
        break;
    case RemoteFileDialog::Reject:
        if (QPushButton *button = qFileDialogUi->buttonBox->button(QDialogButtonBox::Cancel))
            button->setText(text);
        break;
    }
}

/*!
    Sets the \a text shown in the filedialog in the specified \a label.
*/

void RemoteFileDialog::setLabelText(DialogLabel label, const QString &text)
{
    Q_D(RemoteFileDialog);
    d->options->setLabelText(static_cast<RemoteFileDialogOptions::DialogLabel>(label), text);
    d->setLabelTextControl(label, text);
}

/*!
    Returns the text shown in the filedialog in the specified \a label.
*/
QString RemoteFileDialog::labelText(DialogLabel label) const
{
    Q_D(const RemoteFileDialog);
    QPushButton *button;
    switch (label) {
    case LookIn:
        return d->qFileDialogUi->lookInLabel->text();
    case FileName:
        return d->qFileDialogUi->fileNameLabel->text();
    case FileType:
        return d->qFileDialogUi->fileTypeLabel->text();
    case Accept:
        if (acceptMode() == AcceptOpen)
            button = d->qFileDialogUi->buttonBox->button(QDialogButtonBox::Open);
        else
            button = d->qFileDialogUi->buttonBox->button(QDialogButtonBox::Save);
        if (button)
            return button->text();
        break;
    case Reject:
        button = d->qFileDialogUi->buttonBox->button(QDialogButtonBox::Cancel);
        if (button)
            return button->text();
        break;
    }
    return QString();
}

#if 0
/*!
    This is a convenience static function that returns an existing file
    selected by the user. If the user presses Cancel, it returns a null string.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 8

    The function creates a modal file dialog with the given \a parent widget.
    If \a parent is not 0, the dialog will be shown centered over the parent
    widget.

    The file dialog's working directory will be set to \a dir. If \a dir
    includes a file name, the file will be selected. Only files that match the
    given \a filter are shown. The filter selected is set to \a selectedFilter.
    The parameters \a dir, \a selectedFilter, and \a filter may be empty
    strings. If you want multiple filters, separate them with ';;', for
    example:

    \code
    "Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)"
    \endcode

    The \a options argument holds various options about how to run the dialog,
    see the RemoteFileDialog::Option enum for more information on the flags you can
    pass.

    The dialog's caption is set to \a caption. If \a caption is not specified
    then a default caption will be used.

    On Windows, and \macos, this static function will use the
    native file dialog and not a RemoteFileDialog.

    On Windows the dialog will spin a blocking modal event loop that will not
    dispatch any QTimers, and if \a parent is not 0 then it will position the
    dialog just below the parent's title bar.

    On Unix/X11, the normal behavior of the file dialog is to resolve and
    follow symlinks. For example, if \c{/usr/tmp} is a symlink to \c{/var/tmp},
    the file dialog will change to \c{/var/tmp} after entering \c{/usr/tmp}. If
    \a options includes DontResolveSymlinks, the file dialog will treat
    symlinks as regular directories.

    \warning Do not delete \a parent during the execution of the dialog. If you
    want to do this, you should create the dialog yourself using one of the
    RemoteFileDialog constructors.

    \sa getOpenFileNames(), getSaveFileName(), getExistingDirectory()
*/
QString RemoteFileDialog::getOpenFileName(QWidget *parent,
                               const QString &caption,
                               const QString &dir,
                               const QString &filter,
                               QString *selectedFilter,
                               Options options)
{
    const QStringList schemes = QStringList(QStringLiteral("file"));
    const QUrl selectedUrl = getOpenFileUrl(parent, caption, QUrl::fromLocalFile(dir), filter, selectedFilter, options, schemes);
    return selectedUrl.toLocalFile();
}

/*!
    This is a convenience static function that returns an existing file
    selected by the user. If the user presses Cancel, it returns an
    empty url.

    The function is used similarly to RemoteFileDialog::getOpenFileName(). In
    particular \a parent, \a caption, \a dir, \a filter, \a selectedFilter
    and \a options are used in the exact same way.

    The main difference with RemoteFileDialog::getOpenFileName() comes from
    the ability offered to the user to select a remote file. That's why
    the return type and the type of \a dir is QUrl.

    The \a supportedSchemes argument allows to restrict the type of URLs the
    user will be able to select. It is a way for the application to declare
    the protocols it will support to fetch the file content. An empty list
    means that no restriction is applied (the default).
    Supported for local files ("file" scheme) is implicit and always enabled;
    it is not necessary to include it in the restriction.

    When possible, this static function will use the native file dialog and
    not a RemoteFileDialog. On platforms which don't support selecting remote
    files, Qt will allow to select only local files.

    \sa getOpenFileName(), getOpenFileUrls(), getSaveFileUrl(), getExistingDirectoryUrl()
    \since 5.2
*/
QUrl RemoteFileDialog::getOpenFileUrl(QWidget *parent,
                                 const QString &caption,
                                 const QUrl &dir,
                                 const QString &filter,
                                 QString *selectedFilter,
                                 Options options,
                                 const QStringList &supportedSchemes)
{
    RemoteFileDialogArgs args;
    args.parent = parent;
    args.caption = caption;
    args.directory = RemoteFileDialogPrivate::workingDirectory(dir);
    args.selection = RemoteFileDialogPrivate::initialSelection(dir);
    args.filter = filter;
    args.mode = ExistingFile;
    args.options = options;

    RemoteFileDialog dialog(args);
    dialog.setSupportedSchemes(supportedSchemes);
    if (selectedFilter && !selectedFilter->isEmpty())
        dialog.selectNameFilter(*selectedFilter);
    if (dialog.exec() == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog.selectedNameFilter();
        return dialog.selectedUrls().value(0);
    }
    return QUrl();
}

/*!
    This is a convenience static function that will return one or more existing
    files selected by the user.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 9

    This function creates a modal file dialog with the given \a parent widget.
    If \a parent is not 0, the dialog will be shown centered over the parent
    widget.

    The file dialog's working directory will be set to \a dir. If \a dir
    includes a file name, the file will be selected. The filter is set to
    \a filter so that only those files which match the filter are shown. The
    filter selected is set to \a selectedFilter. The parameters \a dir,
    \a selectedFilter and \a filter may be empty strings. If you need multiple
    filters, separate them with ';;', for instance:

    \code
    "Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)"
    \endcode

    The dialog's caption is set to \a caption. If \a caption is not specified
    then a default caption will be used.

    On Windows, and \macos, this static function will use the
    native file dialog and not a RemoteFileDialog.

    On Windows the dialog will spin a blocking modal event loop that will not
    dispatch any QTimers, and if \a parent is not 0 then it will position the
    dialog just below the parent's title bar.

    On Unix/X11, the normal behavior of the file dialog is to resolve and
    follow symlinks. For example, if \c{/usr/tmp} is a symlink to \c{/var/tmp},
    the file dialog will change to \c{/var/tmp} after entering \c{/usr/tmp}.
    The \a options argument holds various options about how to run the dialog,
    see the RemoteFileDialog::Option enum for more information on the flags you can
    pass.

    \warning Do not delete \a parent during the execution of the dialog. If you
    want to do this, you should create the dialog yourself using one of the
    RemoteFileDialog constructors.

    \sa getOpenFileName(), getSaveFileName(), getExistingDirectory()
*/
QStringList RemoteFileDialog::getOpenFileNames(QWidget *parent,
                                          const QString &caption,
                                          const QString &dir,
                                          const QString &filter,
                                          QString *selectedFilter,
                                          Options options)
{
    const QStringList schemes = QStringList(QStringLiteral("file"));
    const QList<QUrl> selectedUrls = getOpenFileUrls(parent, caption, QUrl::fromLocalFile(dir), filter, selectedFilter, options, schemes);
    QStringList fileNames;
    fileNames.reserve(selectedUrls.size());
    for (const QUrl &url : selectedUrls)
        fileNames << url.toLocalFile();
    return fileNames;
}

/*!
    This is a convenience static function that will return one or more existing
    files selected by the user. If the user presses Cancel, it returns an
    empty list.

    The function is used similarly to RemoteFileDialog::getOpenFileNames(). In
    particular \a parent, \a caption, \a dir, \a filter, \a selectedFilter
    and \a options are used in the exact same way.

    The main difference with RemoteFileDialog::getOpenFileNames() comes from
    the ability offered to the user to select remote files. That's why
    the return type and the type of \a dir are respectively QList<QUrl>
    and QUrl.

    The \a supportedSchemes argument allows to restrict the type of URLs the
    user will be able to select. It is a way for the application to declare
    the protocols it will support to fetch the file content. An empty list
    means that no restriction is applied (the default).
    Supported for local files ("file" scheme) is implicit and always enabled;
    it is not necessary to include it in the restriction.

    When possible, this static function will use the native file dialog and
    not a RemoteFileDialog. On platforms which don't support selecting remote
    files, Qt will allow to select only local files.

    \sa getOpenFileNames(), getOpenFileUrl(), getSaveFileUrl(), getExistingDirectoryUrl()
    \since 5.2
*/
QList<QUrl> RemoteFileDialog::getOpenFileUrls(QWidget *parent,
                                         const QString &caption,
                                         const QUrl &dir,
                                         const QString &filter,
                                         QString *selectedFilter,
                                         Options options,
                                         const QStringList &supportedSchemes)
{
    RemoteFileDialogArgs args;
    args.parent = parent;
    args.caption = caption;
    args.directory = RemoteFileDialogPrivate::workingDirectory(dir);
    args.selection = RemoteFileDialogPrivate::initialSelection(dir);
    args.filter = filter;
    args.mode = ExistingFiles;
    args.options = options;

    RemoteFileDialog dialog(args);
    dialog.setSupportedSchemes(supportedSchemes);
    if (selectedFilter && !selectedFilter->isEmpty())
        dialog.selectNameFilter(*selectedFilter);
    if (dialog.exec() == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog.selectedNameFilter();
        return dialog.selectedUrls();
    }
    return QList<QUrl>();
}

/*!
    This is a convenience static function that will return a file name selected
    by the user. The file does not have to exist.

    It creates a modal file dialog with the given \a parent widget. If
    \a parent is not 0, the dialog will be shown centered over the parent
    widget.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 11

    The file dialog's working directory will be set to \a dir. If \a dir
    includes a file name, the file will be selected. Only files that match the
    \a filter are shown. The filter selected is set to \a selectedFilter. The
    parameters \a dir, \a selectedFilter, and \a filter may be empty strings.
    Multiple filters are separated with ';;'. For instance:

    \code
    "Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)"
    \endcode

    The \a options argument holds various options about how to run the dialog,
    see the RemoteFileDialog::Option enum for more information on the flags you can
    pass.

    The default filter can be chosen by setting \a selectedFilter to the
    desired value.

    The dialog's caption is set to \a caption. If \a caption is not specified,
    a default caption will be used.

    On Windows, and \macos, this static function will use the
    native file dialog and not a RemoteFileDialog.

    On Windows the dialog will spin a blocking modal event loop that will not
    dispatch any QTimers, and if \a parent is not 0 then it will position the
    dialog just below the parent's title bar. On \macos, with its native file
    dialog, the filter argument is ignored.

    On Unix/X11, the normal behavior of the file dialog is to resolve and
    follow symlinks. For example, if \c{/usr/tmp} is a symlink to \c{/var/tmp},
    the file dialog will change to \c{/var/tmp} after entering \c{/usr/tmp}. If
    \a options includes DontResolveSymlinks the file dialog will treat symlinks
    as regular directories.

    \warning Do not delete \a parent during the execution of the dialog. If you
    want to do this, you should create the dialog yourself using one of the
    RemoteFileDialog constructors.

    \sa getOpenFileName(), getOpenFileNames(), getExistingDirectory()
*/
QString RemoteFileDialog::getSaveFileName(QWidget *parent,
                                     const QString &caption,
                                     const QString &dir,
                                     const QString &filter,
                                     QString *selectedFilter,
                                     Options options)
{
    const QStringList schemes = QStringList(QStringLiteral("file"));
    const QUrl selectedUrl = getSaveFileUrl(parent, caption, QUrl::fromLocalFile(dir), filter, selectedFilter, options, schemes);
    return selectedUrl.toLocalFile();
}

/*!
    This is a convenience static function that returns a file selected by
    the user. The file does not have to exist. If the user presses Cancel,
    it returns an empty url.

    The function is used similarly to RemoteFileDialog::getSaveFileName(). In
    particular \a parent, \a caption, \a dir, \a filter, \a selectedFilter
    and \a options are used in the exact same way.

    The main difference with RemoteFileDialog::getSaveFileName() comes from
    the ability offered to the user to select a remote file. That's why
    the return type and the type of \a dir is QUrl.

    The \a supportedSchemes argument allows to restrict the type of URLs the
    user will be able to select. It is a way for the application to declare
    the protocols it will support to save the file content. An empty list
    means that no restriction is applied (the default).
    Supported for local files ("file" scheme) is implicit and always enabled;
    it is not necessary to include it in the restriction.

    When possible, this static function will use the native file dialog and
    not a RemoteFileDialog. On platforms which don't support selecting remote
    files, Qt will allow to select only local files.

    \sa getSaveFileName(), getOpenFileUrl(), getOpenFileUrls(), getExistingDirectoryUrl()
    \since 5.2
*/
QUrl RemoteFileDialog::getSaveFileUrl(QWidget *parent,
                                 const QString &caption,
                                 const QUrl &dir,
                                 const QString &filter,
                                 QString *selectedFilter,
                                 Options options,
                                 const QStringList &supportedSchemes)
{
    RemoteFileDialogArgs args;
    args.parent = parent;
    args.caption = caption;
    args.directory = RemoteFileDialogPrivate::workingDirectory(dir);
    args.selection = RemoteFileDialogPrivate::initialSelection(dir);
    args.filter = filter;
    args.mode = AnyFile;
    args.options = options;

    RemoteFileDialog dialog(args);
    dialog.setSupportedSchemes(supportedSchemes);
    dialog.setAcceptMode(AcceptSave);
    if (selectedFilter && !selectedFilter->isEmpty())
        dialog.selectNameFilter(*selectedFilter);
    if (dialog.exec() == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog.selectedNameFilter();
        return dialog.selectedUrls().value(0);
    }
    return QUrl();
}

/*!
    This is a convenience static function that will return an existing
    directory selected by the user.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 12

    This function creates a modal file dialog with the given \a parent widget.
    If \a parent is not 0, the dialog will be shown centered over the parent
    widget.

    The dialog's working directory is set to \a dir, and the caption is set to
    \a caption. Either of these may be an empty string in which case the
    current directory and a default caption will be used respectively.

    The \a options argument holds various options about how to run the dialog,
    see the RemoteFileDialog::Option enum for more information on the flags you can
    pass. To ensure a native file dialog, \l{RemoteFileDialog::}{ShowDirsOnly} must
    be set.

    On Windows and \macos, this static function will use the
    native file dialog and not a RemoteFileDialog. However, the native Windows file
    dialog does not support displaying files in the directory chooser. You need
    to pass \l{RemoteFileDialog::}{DontUseNativeDialog} to display files using a
    RemoteFileDialog.

    On Unix/X11, the normal behavior of the file dialog is to resolve and
    follow symlinks. For example, if \c{/usr/tmp} is a symlink to \c{/var/tmp},
    the file dialog will change to \c{/var/tmp} after entering \c{/usr/tmp}. If
    \a options includes DontResolveSymlinks, the file dialog will treat
    symlinks as regular directories.

    On Windows, the dialog will spin a blocking modal event loop that will not
    dispatch any QTimers, and if \a parent is not 0 then it will position the
    dialog just below the parent's title bar.

    \warning Do not delete \a parent during the execution of the dialog. If you
    want to do this, you should create the dialog yourself using one of the
    RemoteFileDialog constructors.

    \sa getOpenFileName(), getOpenFileNames(), getSaveFileName()
*/
QString RemoteFileDialog::getExistingDirectory(QWidget *parent,
                                          const QString &caption,
                                          const QString &dir,
                                          Options options)
{
    const QStringList schemes = QStringList(QStringLiteral("file"));
    const QUrl selectedUrl = getExistingDirectoryUrl(parent, caption, QUrl::fromLocalFile(dir), options, schemes);
    return selectedUrl.toLocalFile();
}

/*!
    This is a convenience static function that will return an existing
    directory selected by the user. If the user presses Cancel, it
    returns an empty url.

    The function is used similarly to RemoteFileDialog::getExistingDirectory().
    In particular \a parent, \a caption, \a dir and \a options are used
    in the exact same way.

    The main difference with RemoteFileDialog::getExistingDirectory() comes from
    the ability offered to the user to select a remote directory. That's why
    the return type and the type of \a dir is QUrl.

    The \a supportedSchemes argument allows to restrict the type of URLs the
    user will be able to select. It is a way for the application to declare
    the protocols it will support to fetch the file content. An empty list
    means that no restriction is applied (the default).
    Supported for local files ("file" scheme) is implicit and always enabled;
    it is not necessary to include it in the restriction.

    When possible, this static function will use the native file dialog and
    not a RemoteFileDialog. On platforms which don't support selecting remote
    files, Qt will allow to select only local files.

    \sa getExistingDirectory(), getOpenFileUrl(), getOpenFileUrls(), getSaveFileUrl()
    \since 5.2
*/
QUrl RemoteFileDialog::getExistingDirectoryUrl(QWidget *parent,
                                          const QString &caption,
                                          const QUrl &dir,
                                          Options options,
                                          const QStringList &supportedSchemes)
{
    RemoteFileDialogArgs args;
    args.parent = parent;
    args.caption = caption;
    args.directory = RemoteFileDialogPrivate::workingDirectory(dir);
    args.mode = Directory;
    args.options = options;

    RemoteFileDialog dialog(args);
    dialog.setSupportedSchemes(supportedSchemes);
    if (dialog.exec() == QDialog::Accepted)
        return dialog.selectedUrls().value(0);
    return QUrl();
}

inline static QUrl _qt_get_directory(const QUrl &url)
{
    if (url.isLocalFile()) {
        QFileInfo info = QFileInfo(QDir::current(), url.toLocalFile());
        if (info.exists() && info.isDir())
            return QUrl::fromLocalFile(QDir::cleanPath(info.absoluteFilePath()));
        info.setFile(info.absolutePath());
        if (info.exists() && info.isDir())
            return QUrl::fromLocalFile(info.absoluteFilePath());
        return QUrl();
    } else {
        return url;
    }
}
/*
    Get the initial directory URL

    \sa initialSelection()
 */
QUrl RemoteFileDialogPrivate::workingDirectory(const QUrl &url)
{
    if (!url.isEmpty()) {
        QUrl directory = _qt_get_directory(url);
        if (!directory.isEmpty())
            return directory;
    }
    QUrl directory = _qt_get_directory(*lastVisitedDir());
    if (!directory.isEmpty())
        return directory;
    return QUrl::fromLocalFile(model->workingDirectory());
}
#endif

/*
    Get the initial selection given a path.  The initial directory
    can contain both the initial directory and initial selection
    /home/user/foo.txt

    \sa workingDirectory()
 */
QString RemoteFileDialogPrivate::initialSelection(const QUrl &url)
{
    if (url.isEmpty())
        return QString();
    if (url.scheme() == QLatin1String("home")) {
        return QString();
    }
    // With remote URLs we can only assume.
    return url.fileName();
}

/*!
 \reimp
*/
void RemoteFileDialog::done(int result)
{
    Q_D(RemoteFileDialog);

    QDialog::done(result);

    if (d->receiverToDisconnectOnClose) {
        disconnect(this, d->signalToDisconnectOnClose, d->receiverToDisconnectOnClose, d->memberToDisconnectOnClose);
        d->receiverToDisconnectOnClose = 0;
    }
    d->memberToDisconnectOnClose.clear();
    d->signalToDisconnectOnClose.clear();
}

/*!
 \reimp
*/
void RemoteFileDialog::accept()
{
    Q_D(RemoteFileDialog);
    const QStringList files = selectedFiles();
    if (files.isEmpty())
        return;
    QString lineEditText = d->lineEdit()->text();
    // "hidden feature" type .. and then enter, and it will move up a dir
    // special case for ".."
    if (lineEditText == QLatin1String("..")) {
        d->_q_navigateToParent();
        const QSignalBlocker blocker(d->qFileDialogUi->fileNameEdit);
        d->lineEdit()->selectAll();
        return;
    }

    switch (fileMode()) {
    case Directory: {
        QString fn = files.first();
        QModelIndex idx = m_model->fsIndex(fn);
        if (!idx.isValid())
            return;
        if (!m_model->isDir(idx))
            return;
        d->emitFilesSelected(files);
        QDialog::accept();
        return;
    }

    case AnyFile: {
        QString fn = files.first();
        QModelIndex idx = m_model->fsIndex(fn);
        if (idx.isValid() && m_model->isDir(idx)) {
            setDirectory(fn);
            return;
        }
#if 0
        if (!info.exists()) {
            int maxNameLength = d->maxNameLength(info.path());
            if (maxNameLength >= 0 && info.fileName().length() > maxNameLength)
                return;
        }
#endif
        if (!idx.isValid() || !confirmOverwrite() || acceptMode() == AcceptOpen) {
            d->emitFilesSelected(QStringList(fn));
            QDialog::accept();
#if QT_CONFIG(messagebox)
        } else {
            if (QMessageBox::warning(this, windowTitle(), tr("%1 already exists.\nDo you want to replace it?").arg(fn),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
                d->emitFilesSelected(QStringList(fn));
                QDialog::accept();
            }
#endif
        }
        return;
    }

    case ExistingFile:
    case ExistingFiles:
        for (const auto &file: files) {
            QString fn = file;
            QModelIndex idx = m_model->fsIndex(fn);
            if (!idx.isValid()) {
#if QT_CONFIG(messagebox)
                QString message = tr("%1\nFile not found.\nPlease verify the "
                                     "correct file name was given.");
                QMessageBox::warning(this, windowTitle(), message.arg(fn));
#endif // QT_CONFIG(messagebox)
                return;
            }
            if (m_model->isDir(idx)) {
                setDirectory(fn);
                d->lineEdit()->clear();
                return;
            }
        }
        d->emitFilesSelected(files);
        QDialog::accept();
        return;
    }
}

#ifndef QT_NO_SETTINGS
void RemoteFileDialogPrivate::saveSettings()
{
    Q_Q(RemoteFileDialog);
    QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
    settings.beginGroup(QLatin1String("FileDialog"));

    settings.setValue(QLatin1String("sidebarWidth"), qFileDialogUi->splitter->sizes().constFirst());
    settings.setValue(QLatin1String("treeViewHeader"), qFileDialogUi->treeView->header()->saveState());
    const QMetaEnum &viewModeMeta = q->metaObject()->enumerator(q->metaObject()->indexOfEnumerator("ViewMode"));
    settings.setValue(QLatin1String("viewMode"), QLatin1String(viewModeMeta.key(q->viewMode())));
    settings.setValue(QLatin1String("qtVersion"), QLatin1String(QT_VERSION_STR));

    settings.beginGroup(QLatin1String("remote_") + model->identifier());
    settings.setValue(QLatin1String("shortcuts"), QUrl::toStringList(qFileDialogUi->sidebar->urls()));
    QStringList historyUrls;
    const QStringList history = q->history();
    historyUrls.reserve(history.size());
    for (const QString &path: history)
        historyUrls << QUrl::fromLocalFile(path).toString();
    settings.setValue(QLatin1String("history"), historyUrls);
    settings.setValue(QLatin1String("lastVisited"), lastVisitedDir()->toString());
}

bool RemoteFileDialogPrivate::restoreFromSettings()
{
    Q_Q(RemoteFileDialog);
    QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
    if (!settings.childGroups().contains(QLatin1String("FileDialog")))
        return false;
    settings.beginGroup(QLatin1String("FileDialog"));

    QByteArray viewModeStr = settings.value(QLatin1String("viewMode")).toString().toLatin1();
    const QMetaEnum &viewModeMeta = q->metaObject()->enumerator(q->metaObject()->indexOfEnumerator("ViewMode"));
    bool ok = false;
    int viewMode = viewModeMeta.keyToValue(viewModeStr.constData(), &ok);
    if (!ok)
        viewMode = RemoteFileDialog::List;
    q->setViewMode(static_cast<RemoteFileDialog::ViewMode>(viewMode));
    headerData = settings.value(QLatin1String("treeViewHeader")).toByteArray();
    int sidebarWidth = settings.value(QLatin1String("sidebarWidth"), -1).toInt();

    settings.beginGroup(QLatin1String("remote_") + model->identifier());
    q->setDirectoryUrl(lastVisitedDir()->isEmpty() ? settings.value(QLatin1String("lastVisited")).toUrl()
                                                   : *lastVisitedDir());
    sidebarUrls = QUrl::fromStringList(settings.value(QLatin1String("shortcuts")).toStringList());

    QStringList history;
    const auto urlStrings = settings.value(QLatin1String("history")).toStringList();
    for (const QString &urlStr: urlStrings) {
        QUrl url(urlStr);
        if (url.isLocalFile())
            history << url.toLocalFile();
    }

    return restoreWidgetState(history, sidebarWidth);
}
#endif // QT_NO_SETTINGS

bool RemoteFileDialogPrivate::restoreWidgetState(QStringList &history, int splitterPosition)
{
    Q_Q(RemoteFileDialog);
    if (splitterPosition >= 0) {
        QList<int> splitterSizes;
        splitterSizes.append(splitterPosition);
        splitterSizes.append(qFileDialogUi->splitter->widget(1)->sizeHint().width());
        qFileDialogUi->splitter->setSizes(splitterSizes);
    } else {
        if (!qFileDialogUi->splitter->restoreState(splitterState))
            return false;
        QList<int> list = qFileDialogUi->splitter->sizes();
        if (list.count() >= 2 && (list.at(0) == 0 || list.at(1) == 0)) {
            for (int i = 0; i < list.count(); ++i)
                list[i] = qFileDialogUi->splitter->widget(i)->sizeHint().width();
            qFileDialogUi->splitter->setSizes(list);
        }
    }

    q->setSidebarUrls(sidebarUrls);

    static const int MaxHistorySize = 5;
    if (history.size() > MaxHistorySize)
        history.erase(history.begin(), history.end() - MaxHistorySize);
    q->setHistory(history);

    QHeaderView *headerView = qFileDialogUi->treeView->header();
    if (!headerView->restoreState(headerData))
        return false;

    QList<QAction *> actions = headerView->actions();
    QAbstractItemModel *abstractModel = model;
#ifndef QT_NO_PROXYMODEL
    if (proxyModel)
        abstractModel = proxyModel;
#endif
    int total = qMin(abstractModel->columnCount(QModelIndex()), actions.count() + 1);
    for (int i = 1; i < total; ++i)
        actions.at(i - 1)->setChecked(!headerView->isSectionHidden(i));

    return true;
}

/*!
    \internal

    Create widgets, layout and set default values
*/
void RemoteFileDialogPrivate::init(const QUrl &directory, const QString &nameFilter, const QString &caption)
{
    Q_Q(RemoteFileDialog);
    if (!caption.isEmpty()) {
        useDefaultCaption = false;
        setWindowTitle = caption;
        q->setWindowTitle(caption);
    }

    createWidgets();

    q->setAcceptMode(RemoteFileDialog::AcceptOpen);
    q->setFileMode(RemoteFileDialog::AnyFile);
    if (!nameFilter.isEmpty())
        q->setNameFilter(nameFilter);
    q->setDirectoryUrl(QUrl::fromLocalFile(model->workingDirectory()));
    if (directory.isLocalFile())
        q->selectFile(initialSelection(directory));
    else
        q->selectUrl(directory);

#ifndef QT_NO_SETTINGS
    // Try to restore from the FileDialog settings group
    restoreFromSettings();
#endif

#if defined(Q_EMBEDDED_SMALLSCREEN)
    qFileDialogUi->lookInLabel->setVisible(false);
    qFileDialogUi->fileNameLabel->setVisible(false);
    qFileDialogUi->fileTypeLabel->setVisible(false);
    qFileDialogUi->sidebar->hide();
#endif

    const QSize sizeHint = q->sizeHint();
    if (sizeHint.isValid())
        q->resize(sizeHint);
}

/*!
    \internal

    Create the widgets, set properties and connections
*/
void RemoteFileDialogPrivate::createWidgets()
{
    if (qFileDialogUi)
        return;
    Q_Q(RemoteFileDialog);

    // This function is sometimes called late (e.g as a fallback from setVisible). In that case we
    // need to ensure that the following UI code (setupUI in particular) doesn't reset any explicitly
    // set window state or geometry.
    QSize preSize = q->testAttribute(Qt::WA_Resized) ? q->size() : QSize();
    Qt::WindowStates preState = q->windowState();

    qFileDialogUi.reset(new Ui_RemoteFileDialog());
    qFileDialogUi->setupUi(q);

    model = q->m_model;
    model->setFilter(options->filter());
    model->setObjectName(QLatin1String("remote_filesystem_model"));
    model->setDisableRecursiveSort(true);
#if 0
    if (QPlatformFileDialogHelper *helper = platformFileDialogHelper())
        model->setNameFilterDisables(helper->defaultNameFilterDisables());
    else
        model->setNameFilterDisables(false);
#else
#ifdef Q_OS_MACOS
    model->setNameFilterDisables(true);
#else
    model->setNameFilterDisables(false);
#endif
#endif
    RemoteFileDialog::connect(model, SIGNAL(fileRenamed(QString, QString, QString)), q,
                              SLOT(_q_fileRenamed(QString, QString, QString)));
    RemoteFileDialog::connect(model, SIGNAL(rootPathChanged(QString)), q, SLOT(_q_pathChanged(QString)));
    RemoteFileDialog::connect(model, SIGNAL(rowsInserted(QModelIndex, int, int)), q,
                              SLOT(_q_rowsInserted(QModelIndex)));
    RemoteFileDialog::connect(model, SIGNAL(directoryLoaded(QString)), q, SLOT(_q_directoryLoaded(QString)));
    model->setReadOnly(false);

    QList<QUrl> initialBookmarks;
    qFileDialogUi->sidebar->setModelAndUrls(model, initialBookmarks);
    q->setSidebarUrls(initialBookmarks);
    RemoteFileDialog::connect(qFileDialogUi->sidebar, SIGNAL(goToUrl(QUrl)), q, SLOT(_q_goToUrl(QUrl)));

    QObject::connect(qFileDialogUi->buttonBox, SIGNAL(accepted()), q, SLOT(accept()));
    QObject::connect(qFileDialogUi->buttonBox, SIGNAL(rejected()), q, SLOT(reject()));

    qFileDialogUi->lookInCombo->setFileDialogPrivate(this);
    QObject::connect(qFileDialogUi->lookInCombo, SIGNAL(activated(QString)), q, SLOT(_q_goToDirectory(QString)));

    qFileDialogUi->lookInCombo->setInsertPolicy(QComboBox::NoInsert);
    qFileDialogUi->lookInCombo->setDuplicatesEnabled(false);

    // filename
    qFileDialogUi->fileNameEdit->setFileDialogPrivate(this);
#ifndef QT_NO_SHORTCUT
    qFileDialogUi->fileNameLabel->setBuddy(qFileDialogUi->fileNameEdit);
#endif
    completer = new RemoteFSCompleter(model, q);
    qFileDialogUi->fileNameEdit->setCompleter(completer);

    qFileDialogUi->fileNameEdit->setInputMethodHints(Qt::ImhNoPredictiveText);

    QObject::connect(qFileDialogUi->fileNameEdit, SIGNAL(textChanged(QString)), q,
                     SLOT(_q_autoCompleteFileName(QString)));
    QObject::connect(qFileDialogUi->fileNameEdit, SIGNAL(textChanged(QString)), q, SLOT(_q_updateOkButton()));

    QObject::connect(qFileDialogUi->fileNameEdit, SIGNAL(returnPressed()), q, SLOT(accept()));

    // filetype
    qFileDialogUi->fileTypeCombo->setDuplicatesEnabled(false);
    qFileDialogUi->fileTypeCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    qFileDialogUi->fileTypeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QObject::connect(qFileDialogUi->fileTypeCombo, SIGNAL(activated(int)), q, SLOT(_q_useNameFilter(int)));
    QObject::connect(qFileDialogUi->fileTypeCombo, SIGNAL(activated(QString)), q, SIGNAL(filterSelected(QString)));

    qFileDialogUi->listView->setFileDialogPrivate(this);
    qFileDialogUi->listView->setModel(model);
    QObject::connect(qFileDialogUi->listView, SIGNAL(activated(QModelIndex)), q, SLOT(_q_enterDirectory(QModelIndex)));
    QObject::connect(qFileDialogUi->listView, SIGNAL(customContextMenuRequested(QPoint)), q,
                     SLOT(_q_showContextMenu(QPoint)));
    qFileDialogUi->treeView->setFileDialogPrivate(this);
    qFileDialogUi->treeView->setModel(model);
    QHeaderView *treeHeader = qFileDialogUi->treeView->header();
    QFontMetrics fm(q->font());
    treeHeader->resizeSection(0, fm.boundingRect(QLatin1String("wwwwwwwwwwwwwwwwwwwwwwwwww")).width());
    treeHeader->resizeSection(1, fm.boundingRect(QLatin1String("128.88 GB")).width());
    treeHeader->resizeSection(2, fm.boundingRect(QLatin1String("mp3Folder")).width());
    treeHeader->resizeSection(3, fm.boundingRect(QLatin1String("10/29/81 02:02PM")).width());
    treeHeader->setContextMenuPolicy(Qt::ActionsContextMenu);

    QActionGroup *showActionGroup = new QActionGroup(q);
    showActionGroup->setExclusive(false);
    QObject::connect(showActionGroup, SIGNAL(triggered(QAction *)), q, SLOT(_q_showHeader(QAction *)));
    ;

    QAbstractItemModel *abstractModel = model;
#ifndef QT_NO_PROXYMODEL
    if (proxyModel)
        abstractModel = proxyModel;
#endif
    for (int i = 1; i < abstractModel->columnCount(QModelIndex()); ++i) {
        QAction *showHeader = new QAction(showActionGroup);
        showHeader->setCheckable(true);
        showHeader->setChecked(true);
        treeHeader->addAction(showHeader);
    }

    QScopedPointer<QItemSelectionModel> selModel(qFileDialogUi->treeView->selectionModel());
    qFileDialogUi->treeView->setSelectionModel(qFileDialogUi->listView->selectionModel());

    QObject::connect(qFileDialogUi->treeView, SIGNAL(activated(QModelIndex)), q, SLOT(_q_enterDirectory(QModelIndex)));
    QObject::connect(qFileDialogUi->treeView, SIGNAL(customContextMenuRequested(QPoint)), q,
                     SLOT(_q_showContextMenu(QPoint)));

    // Selections
    QItemSelectionModel *selections = qFileDialogUi->listView->selectionModel();
    QObject::connect(selections, SIGNAL(selectionChanged(QItemSelection, QItemSelection)), q,
                     SLOT(_q_selectionChanged()));
    QObject::connect(selections, SIGNAL(currentChanged(QModelIndex, QModelIndex)), q,
                     SLOT(_q_currentChanged(QModelIndex)));
    qFileDialogUi->splitter->setStretchFactor(qFileDialogUi->splitter->indexOf(qFileDialogUi->splitter->widget(1)),
                                              QSizePolicy::Expanding);

    createToolButtons();
    createMenuActions();

#ifndef QT_NO_SETTINGS
    // Try to restore from the FileDialog settings group
    restoreFromSettings();
#endif

    // Initial widget states from options
    q->setFileMode(static_cast<RemoteFileDialog::FileMode>(options->fileMode()));
    q->setAcceptMode(static_cast<RemoteFileDialog::AcceptMode>(options->acceptMode()));
    q->setViewMode(static_cast<RemoteFileDialog::ViewMode>(options->viewMode()));
    q->setOptions(static_cast<RemoteFileDialog::Options>(static_cast<int>(options->options())));
    if (!options->sidebarUrls().isEmpty())
        q->setSidebarUrls(options->sidebarUrls());
    q->setDirectoryUrl(options->initialDirectory());
#ifndef QT_NO_MIMETYPE
    if (!options->mimeTypeFilters().isEmpty())
        q->setMimeTypeFilters(options->mimeTypeFilters());
    else
#endif
        if (!options->nameFilters().isEmpty())
        q->setNameFilters(options->nameFilters());
    q->selectNameFilter(options->initiallySelectedNameFilter());
    q->setDefaultSuffix(options->defaultSuffix());
    q->setHistory(options->history());
    const auto initiallySelectedFiles = options->initiallySelectedFiles();
    if (initiallySelectedFiles.size() == 1)
        q->selectFile(initiallySelectedFiles.first().fileName());
    for (const QUrl &url: initiallySelectedFiles)
        q->selectUrl(url);
    lineEdit()->selectAll();
    _q_updateOkButton();
    retranslateStrings();
    q->resize(preSize.isValid() ? preSize : q->sizeHint());
    q->setWindowState(preState);
}

void RemoteFileDialogPrivate::_q_showHeader(QAction *action)
{
    Q_Q(RemoteFileDialog);
    QActionGroup *actionGroup = qobject_cast<QActionGroup *>(q->sender());
    qFileDialogUi->treeView->header()->setSectionHidden(actionGroup->actions().indexOf(action) + 1,
                                                        !action->isChecked());
}

#ifndef QT_NO_PROXYMODEL
/*!
    \since 4.3

    Sets the model for the views to the given \a proxyModel.  This is useful if you
    want to modify the underlying model; for example, to add columns, filter
    data or add drives.

    Any existing proxy model will be removed, but not deleted.  The file dialog
    will take ownership of the \a proxyModel.

    \sa proxyModel()
*/
void RemoteFileDialog::setProxyModel(QAbstractProxyModel *proxyModel)
{
    Q_D(RemoteFileDialog);
    if ((!proxyModel && !d->proxyModel) || (proxyModel == d->proxyModel))
        return;

    QModelIndex idx = d->rootIndex();
    if (d->proxyModel) {
        disconnect(d->proxyModel, SIGNAL(rowsInserted(QModelIndex, int, int)), this,
                   SLOT(_q_rowsInserted(QModelIndex)));
    } else {
        disconnect(d->model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(_q_rowsInserted(QModelIndex)));
    }

    if (proxyModel != 0) {
        proxyModel->setParent(this);
        d->proxyModel = proxyModel;
        proxyModel->setSourceModel(d->model);
        d->qFileDialogUi->listView->setModel(d->proxyModel);
        d->qFileDialogUi->treeView->setModel(d->proxyModel);
        d->completer->setModel(d->proxyModel);
        d->completer->proxyModel = d->proxyModel;
        connect(d->proxyModel, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(_q_rowsInserted(QModelIndex)));
    } else {
        d->proxyModel = 0;
        d->qFileDialogUi->listView->setModel(d->model);
        d->qFileDialogUi->treeView->setModel(d->model);
        d->completer->setModel(d->model);
        d->completer->sourceModel = d->model;
        d->completer->proxyModel = 0;
        connect(d->model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(_q_rowsInserted(QModelIndex)));
    }
    QScopedPointer<QItemSelectionModel> selModel(d->qFileDialogUi->treeView->selectionModel());
    d->qFileDialogUi->treeView->setSelectionModel(d->qFileDialogUi->listView->selectionModel());

    d->setRootIndex(idx);

    // reconnect selection
    QItemSelectionModel *selections = d->qFileDialogUi->listView->selectionModel();
    QObject::connect(selections, SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
                     SLOT(_q_selectionChanged()));
    QObject::connect(selections, SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
                     SLOT(_q_currentChanged(QModelIndex)));
}

/*!
    Returns the proxy model used by the file dialog.  By default no proxy is set.

    \sa setProxyModel()
*/
QAbstractProxyModel *RemoteFileDialog::proxyModel() const
{
    Q_D(const RemoteFileDialog);
    return d->proxyModel;
}
#endif // QT_NO_PROXYMODEL

/*!
    \internal

    Create tool buttons, set properties and connections
*/
void RemoteFileDialogPrivate::createToolButtons()
{
    Q_Q(RemoteFileDialog);
    qFileDialogUi->backButton->setIcon(q->style()->standardIcon(QStyle::SP_ArrowBack, 0, q));
    qFileDialogUi->backButton->setAutoRaise(true);
    qFileDialogUi->backButton->setEnabled(false);
    QObject::connect(qFileDialogUi->backButton, SIGNAL(clicked()), q, SLOT(_q_navigateBackward()));

    qFileDialogUi->forwardButton->setIcon(q->style()->standardIcon(QStyle::SP_ArrowForward, 0, q));
    qFileDialogUi->forwardButton->setAutoRaise(true);
    qFileDialogUi->forwardButton->setEnabled(false);
    QObject::connect(qFileDialogUi->forwardButton, SIGNAL(clicked()), q, SLOT(_q_navigateForward()));

    qFileDialogUi->toParentButton->setIcon(q->style()->standardIcon(QStyle::SP_FileDialogToParent, 0, q));
    qFileDialogUi->toParentButton->setAutoRaise(true);
    qFileDialogUi->toParentButton->setEnabled(false);
    QObject::connect(qFileDialogUi->toParentButton, SIGNAL(clicked()), q, SLOT(_q_navigateToParent()));

    qFileDialogUi->listModeButton->setIcon(q->style()->standardIcon(QStyle::SP_FileDialogListView, 0, q));
    qFileDialogUi->listModeButton->setAutoRaise(true);
    qFileDialogUi->listModeButton->setDown(true);
    QObject::connect(qFileDialogUi->listModeButton, SIGNAL(clicked()), q, SLOT(_q_showListView()));

    qFileDialogUi->detailModeButton->setIcon(q->style()->standardIcon(QStyle::SP_FileDialogDetailedView, 0, q));
    qFileDialogUi->detailModeButton->setAutoRaise(true);
    QObject::connect(qFileDialogUi->detailModeButton, SIGNAL(clicked()), q, SLOT(_q_showDetailsView()));

    QSize toolSize(qFileDialogUi->fileNameEdit->sizeHint().height(), qFileDialogUi->fileNameEdit->sizeHint().height());
    qFileDialogUi->backButton->setFixedSize(toolSize);
    qFileDialogUi->listModeButton->setFixedSize(toolSize);
    qFileDialogUi->detailModeButton->setFixedSize(toolSize);
    qFileDialogUi->forwardButton->setFixedSize(toolSize);
    qFileDialogUi->toParentButton->setFixedSize(toolSize);

    qFileDialogUi->newFolderButton->setIcon(q->style()->standardIcon(QStyle::SP_FileDialogNewFolder, 0, q));
    qFileDialogUi->newFolderButton->setFixedSize(toolSize);
    qFileDialogUi->newFolderButton->setAutoRaise(true);
    qFileDialogUi->newFolderButton->setEnabled(false);
    QObject::connect(qFileDialogUi->newFolderButton, SIGNAL(clicked()), q, SLOT(_q_createDirectory()));
}

/*!
    \internal

    Create actions which will be used in the right click.
*/
void RemoteFileDialogPrivate::createMenuActions()
{
    Q_Q(RemoteFileDialog);

    QAction *goHomeAction = new QAction(q);
#ifndef QT_NO_SHORTCUT
    goHomeAction->setShortcut(Qt::CTRL + Qt::Key_H + Qt::SHIFT);
#endif
    QObject::connect(goHomeAction, SIGNAL(triggered()), q, SLOT(_q_goHome()));
    q->addAction(goHomeAction);

    // ### TODO add Desktop & Computer actions

    QAction *goToParent = new QAction(q);
    goToParent->setObjectName(QLatin1String("qt_goto_parent_action"));
#ifndef QT_NO_SHORTCUT
    goToParent->setShortcut(Qt::CTRL + Qt::UpArrow);
#endif
    QObject::connect(goToParent, SIGNAL(triggered()), q, SLOT(_q_navigateToParent()));
    q->addAction(goToParent);

    showHiddenAction = new QAction(q);
    showHiddenAction->setObjectName(QLatin1String("qt_show_hidden_action"));
    showHiddenAction->setCheckable(true);
    QObject::connect(showHiddenAction, SIGNAL(triggered()), q, SLOT(_q_showHidden()));

    newFolderAction = new QAction(q);
    newFolderAction->setObjectName(QLatin1String("qt_new_folder_action"));
    QObject::connect(newFolderAction, SIGNAL(triggered()), q, SLOT(_q_createDirectory()));
}

void RemoteFileDialogPrivate::_q_goHome()
{
    Q_Q(RemoteFileDialog);
    q->setDirectory(model->homePath().toString());
}

/*!
    \internal

    Update history with new path, buttons, and combo
*/
void RemoteFileDialogPrivate::_q_pathChanged(const QString &newPath)
{
    Q_Q(RemoteFileDialog);
    qFileDialogUi->toParentButton->setEnabled(!model->rootPath().isEmpty());
    QUrl url = QUrl::fromLocalFile(newPath);
    if (newPath.isEmpty())
        url = QUrl(QLatin1String("file:"));
    qFileDialogUi->sidebar->selectUrl(url);
    q->setHistory(qFileDialogUi->lookInCombo->history());

    if (currentHistoryLocation < 0 ||
        currentHistory.value(currentHistoryLocation) != QDir::toNativeSeparators(newPath)) {
        while (currentHistoryLocation >= 0 && currentHistoryLocation + 1 < currentHistory.count()) {
            currentHistory.removeLast();
        }
        currentHistory.append(QDir::toNativeSeparators(newPath));
        ++currentHistoryLocation;
    }
    qFileDialogUi->forwardButton->setEnabled(currentHistory.size() - currentHistoryLocation > 1);
    qFileDialogUi->backButton->setEnabled(currentHistoryLocation > 0);

    if (currentHistoryLocation >= 0)
        qInfo() << "PATH changed to" << currentHistory[currentHistoryLocation];
}

/*!
    \internal

    Navigates to the last directory viewed in the dialog.
*/
void RemoteFileDialogPrivate::_q_navigateBackward()
{
    Q_Q(RemoteFileDialog);
    if (!currentHistory.isEmpty() && currentHistoryLocation > 0) {
        --currentHistoryLocation;
        QString previousHistory = currentHistory.at(currentHistoryLocation);
        q->setDirectory(previousHistory);
    }
}

/*!
    \internal

    Navigates to the last directory viewed in the dialog.
*/
void RemoteFileDialogPrivate::_q_navigateForward()
{
    Q_Q(RemoteFileDialog);
    if (!currentHistory.isEmpty() && currentHistoryLocation < currentHistory.size() - 1) {
        ++currentHistoryLocation;
        QString nextHistory = currentHistory.at(currentHistoryLocation);
        q->setDirectory(nextHistory);
    }
}

/*!
    \internal

    Navigates to the parent directory of the currently displayed directory
    in the dialog.
*/
void RemoteFileDialogPrivate::_q_navigateToParent()
{
    Q_Q(RemoteFileDialog);
    QString newDirectory;
    if (model->rootPath().isEmpty() || model->rootPath() == model->myComputer().toString()) {
        newDirectory = QString();
    } else if (model->isRootDir(model->rootPath())) {
        newDirectory = QString();
    } else {
        newDirectory = model->rootPath();
        while (newDirectory.endsWith('/') && newDirectory.count('/') > 1)
            newDirectory = newDirectory.left(-1);
        newDirectory = newDirectory.section('/', 0, -2);
        if (!newDirectory.contains('/'))
            newDirectory.append('/');
    }
    q->setDirectory(newDirectory);
    emit q->directoryEntered(newDirectory);
}

/*!
    \internal

    Creates a new directory, first asking the user for a suitable name.
*/
void RemoteFileDialogPrivate::_q_createDirectory()
{
    Q_Q(RemoteFileDialog);
    qFileDialogUi->listView->clearSelection();

    QString newFolderString = RemoteFileDialog::tr("New Folder");
    QString folderName = lineEdit()->text();
    QString prefix = q->directory() + QDir::separator();
    if (folderName.isEmpty()) {
        folderName = newFolderString;
        qlonglong suffix = 2;
        QString full = QDir::cleanPath(prefix + folderName);
        QModelIndex idx = model->fsIndex(full);
        while (idx.isValid()) {
            folderName = newFolderString + QString::number(suffix++);
            QString full = QDir::cleanPath(prefix + folderName);
            idx = model->fsIndex(full);
        }
    }

    QModelIndex parent = rootIndex();
    QModelIndex index = model->mkdir(parent, folderName);
    if (!index.isValid())
        return;

    index = select(index);
    if (index.isValid()) {
        qFileDialogUi->treeView->setCurrentIndex(index);
        currentView()->edit(index);
    }
}

void RemoteFileDialogPrivate::_q_showListView()
{
    qFileDialogUi->listModeButton->setDown(true);
    qFileDialogUi->detailModeButton->setDown(false);
    qFileDialogUi->treeView->hide();
    qFileDialogUi->listView->show();
    qFileDialogUi->stackedWidget->setCurrentWidget(qFileDialogUi->listView->parentWidget());
    qFileDialogUi->listView->doItemsLayout();
}

void RemoteFileDialogPrivate::_q_showDetailsView()
{
    qFileDialogUi->listModeButton->setDown(false);
    qFileDialogUi->detailModeButton->setDown(true);
    qFileDialogUi->listView->hide();
    qFileDialogUi->treeView->show();
    qFileDialogUi->stackedWidget->setCurrentWidget(qFileDialogUi->treeView->parentWidget());
    qFileDialogUi->treeView->doItemsLayout();
}

/*!
    \internal

    Show the context menu for the file/dir under position
*/
void RemoteFileDialogPrivate::_q_showContextMenu(const QPoint &position)
{
#if !QT_CONFIG(menu)
    Q_UNUSED(position);
#else
    Q_Q(RemoteFileDialog);
    QAbstractItemView *view = 0;
    if (q->viewMode() == RemoteFileDialog::Detail)
        view = qFileDialogUi->treeView;
    else
        view = qFileDialogUi->listView;
    QModelIndex index = view->indexAt(position);
    index = mapToSource(index.sibling(index.row(), 0));

    QMenu menu(view);
    menu.addAction(showHiddenAction);
    if (qFileDialogUi->newFolderButton->isVisible()) {
        newFolderAction->setEnabled(qFileDialogUi->newFolderButton->isEnabled());
        menu.addAction(newFolderAction);
    }
    menu.exec(view->viewport()->mapToGlobal(position));
#endif // QT_CONFIG(menu)
}

void RemoteFileDialogPrivate::_q_autoCompleteFileName(const QString &text)
{
    if (text.startsWith(QLatin1String("//")) || text.startsWith(QLatin1Char('\\'))) {
        qFileDialogUi->listView->selectionModel()->clearSelection();
        return;
    }

    const QStringList multipleFiles = typedFiles();
    if (multipleFiles.count() > 0) {
        QModelIndexList oldFiles = qFileDialogUi->listView->selectionModel()->selectedRows();
        QVector<QModelIndex> newFiles;
        for (const auto &file: multipleFiles) {
            QModelIndex idx = model->fsIndex(file);
            if (oldFiles.removeAll(idx) == 0)
                newFiles.append(idx);
        }
        for (int i = 0; i < newFiles.count(); ++i)
            select(newFiles.at(i));
        if (lineEdit()->hasFocus())
            for (int i = 0; i < oldFiles.count(); ++i)
                qFileDialogUi->listView->selectionModel()->select(oldFiles.at(i), QItemSelectionModel::Toggle |
                                                                                      QItemSelectionModel::Rows);
    }
}

/*!
    \internal
*/
void RemoteFileDialogPrivate::_q_updateOkButton()
{
    Q_Q(RemoteFileDialog);
    QPushButton *button = qFileDialogUi->buttonBox->button(
        (q->acceptMode() == RemoteFileDialog::AcceptOpen) ? QDialogButtonBox::Open : QDialogButtonBox::Save);
    if (!button)
        return;
    const RemoteFileDialog::FileMode fileMode = q->fileMode();

    bool enableButton = true;
    bool isOpenDirectory = false;

    const QStringList files = q->selectedFiles();
    QString lineEditText = lineEdit()->text();

    if (lineEditText.startsWith(QLatin1String("//")) || lineEditText.startsWith(QLatin1Char('\\'))) {
        button->setEnabled(true);
        updateOkButtonText();
        return;
    }

    if (files.isEmpty() && fileMode != RemoteFileDialog::Directory) {
        enableButton = false;
    } else if (lineEditText == QLatin1String("..")) {
        isOpenDirectory = true;
    } else {
        switch (fileMode) {
        case RemoteFileDialog::Directory: {
            const QString &fn = files.first();
            if (!fn.isEmpty()) {
                QModelIndex idx = model->fsIndex(fn);
                if (!idx.isValid() || !model->isDir(idx))
                    enableButton = false;
            }
            break;
        }
        case RemoteFileDialog::AnyFile: {
            const QString &fn = files.first();
            QString clean = QDir::cleanPath(fn);
            QModelIndex idx = model->fsIndex(clean);
            QString fileDir;
            QString fileName;
            if (model->isDir(idx)) {
                fileDir = QDir::cleanPath(fn);
            } else {
                fileDir = clean.mid(0, fn.lastIndexOf(QLatin1Char('/')));
                fileName = clean.mid(fileDir.length() + 1);
            }
#if 0
            // FIXME
            if (lineEditText.contains(QLatin1String(".."))) {
                fileDir = QDir::cleanPath(fn);
                fileName = QDir::
            }
#endif

            if (fileDir == q->directory() && fileName.isEmpty()) {
                enableButton = false;
                break;
            }
            if (idx.isValid() && model->isDir(idx)) {
                isOpenDirectory = true;
                enableButton = true;
                break;
            }
            if (!idx.isValid()) {
                enableButton = true;
            }
            break;
        }
        case RemoteFileDialog::ExistingFile:
        case RemoteFileDialog::ExistingFiles:
            for (const auto &file: files) {
                QModelIndex idx = model->fsIndex(file);
                if (!idx.isValid()) {
                    enableButton = false;
                    break;
                }
                if (idx.isValid() && model->isDir(idx)) {
                    isOpenDirectory = true;
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    button->setEnabled(enableButton);
    updateOkButtonText(isOpenDirectory);
}

/*!
    \internal
*/
void RemoteFileDialogPrivate::_q_currentChanged(const QModelIndex &index)
{
    _q_updateOkButton();
    emit q_func()->currentChanged(index.data(AbstractFileSystemModel::FilePathRole).toString());
}

/*!
    \internal

    This is called when the user double clicks on a file with the corresponding
    model item \a index.
*/
void RemoteFileDialogPrivate::_q_enterDirectory(const QModelIndex &index)
{
    Q_Q(RemoteFileDialog);
    // My Computer or a directory
    QModelIndex sourceIndex = index.model() == proxyModel ? mapToSource(index) : index;
    QString path = sourceIndex.data(AbstractFileSystemModel::FilePathRole).toString();
    qInfo() << "ENTER DIRECTORY" << path;
    if (path.isEmpty() || model->isDir(sourceIndex)) {
        updateCursor(model->canFetchMore(index));
        model->fetchMore(index);
        const RemoteFileDialog::FileMode fileMode = q->fileMode();
        q->setDirectory(path);
        emit q->directoryEntered(path);
        if (fileMode == RemoteFileDialog::Directory) {
            // ### find out why you have to do both of these.
            lineEdit()->setText(QString());
            lineEdit()->clear();
        }
    } else {
        // Do not accept when shift-clicking to multi-select a file in environments with single-click-activation (KDE)
        if (!q->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, qFileDialogUi->treeView) ||
            q->fileMode() != RemoteFileDialog::ExistingFiles || !(QGuiApplication::keyboardModifiers() & Qt::CTRL)) {
            q->accept();
        }
    }
}

/*!
    \internal

    Changes the file dialog's current directory to the one specified
    by \a path.
*/
void RemoteFileDialogPrivate::_q_goToDirectory(const QString &path)
{
#if QT_CONFIG(messagebox)
    Q_Q(RemoteFileDialog);
#endif
    QModelIndex index = qFileDialogUi->lookInCombo->model()->index(qFileDialogUi->lookInCombo->currentIndex(),
                                                                   qFileDialogUi->lookInCombo->modelColumn(),
                                                                   qFileDialogUi->lookInCombo->rootModelIndex());

    if (!index.isValid()) {
#if QT_CONFIG(messagebox)
        QString message = RemoteFileDialog::tr("%1\nDirectory not found.\nPlease verify the "
                                               "correct directory name was given.");
        QMessageBox::warning(q, q->windowTitle(), message.arg(path));
#endif // QT_CONFIG(messagebox)
        return;
    }

    QString path2 = index.data(UrlRole).toUrl().toLocalFile();
    index = mapFromSource(model->fsIndex(path2));

    _q_enterDirectory(index);
}

/*!
    \internal

    Sets the current name filter to be nameFilter and
    update the qFileDialogUi->fileNameEdit when in AcceptSave mode with the new extension.
*/
void RemoteFileDialogPrivate::_q_useNameFilter(int index)
{
    QStringList nameFilters = options->nameFilters();
    if (index == nameFilters.size()) {
        QAbstractItemModel *comboModel = qFileDialogUi->fileTypeCombo->model();
        nameFilters.append(comboModel->index(comboModel->rowCount() - 1, 0).data().toString());
        options->setNameFilters(nameFilters);
    }

    QString nameFilter = nameFilters.at(index);
    QStringList newNameFilters = cleanFilterList(nameFilter);
    if (q_func()->acceptMode() == RemoteFileDialog::AcceptSave) {
        QString newNameFilterExtension;
        if (newNameFilters.count() > 0)
            newNameFilterExtension = QFileInfo(newNameFilters.at(0)).suffix();

        QString fileName = lineEdit()->text();
        const QString fileNameExtension = QFileInfo(fileName).suffix();
        if (!fileNameExtension.isEmpty() && !newNameFilterExtension.isEmpty()) {
            const int fileNameExtensionLength = fileNameExtension.count();
            fileName.replace(fileName.count() - fileNameExtensionLength, fileNameExtensionLength,
                             newNameFilterExtension);
            qFileDialogUi->listView->clearSelection();
            lineEdit()->setText(fileName);
        }
    }

    model->setNameFilters(newNameFilters);
}

/*!
    \internal

    This is called when the model index corresponding to the current file is changed
    from \a index to \a current.
*/
void RemoteFileDialogPrivate::_q_selectionChanged()
{
    const RemoteFileDialog::FileMode fileMode = q_func()->fileMode();
    const QModelIndexList indexes = qFileDialogUi->listView->selectionModel()->selectedRows();
    bool stripDirs = (fileMode != RemoteFileDialog::Directory);
    bool clearOnDirectory = fileMode != RemoteFileDialog::ExistingFiles && fileMode != RemoteFileDialog::ExistingFile;
    bool haveDirectory = false;

    QStringList allFiles;
    for (const auto &index: indexes) {
        if (stripDirs && model->isDir(mapToSource(index))) {
            haveDirectory = true;
            continue;
        }
        allFiles.append(index.data().toString());
    }
    if (allFiles.count() > 1)
        for (int i = 0; i < allFiles.count(); ++i) {
            allFiles.replace(i, QString(QLatin1Char('"') + allFiles.at(i) + QLatin1Char('"')));
        }

    QString finalFiles = allFiles.join(QLatin1Char(' '));
    if ((!finalFiles.isEmpty() || (haveDirectory && clearOnDirectory)) && !lineEdit()->hasFocus() &&
        lineEdit()->isVisible()) {
        lineEdit()->setText(finalFiles);
    } else {
        _q_updateOkButton();
    }
}

/*!
    \internal

    Includes hidden files and directories in the items displayed in the dialog.
*/
void RemoteFileDialogPrivate::_q_showHidden()
{
    Q_Q(RemoteFileDialog);
    QDir::Filters dirFilters = q->filter();
    dirFilters.setFlag(QDir::Hidden, showHiddenAction->isChecked());
    q->setFilter(dirFilters);
}

/*!
    \internal

    When parent is root and rows have been inserted when none was there before
    then select the first one.
*/
void RemoteFileDialogPrivate::_q_rowsInserted(const QModelIndex &parent)
{
    if (!qFileDialogUi->treeView || parent != qFileDialogUi->treeView->rootIndex() ||
        !qFileDialogUi->treeView->selectionModel() || qFileDialogUi->treeView->selectionModel()->hasSelection() ||
        qFileDialogUi->treeView->model()->rowCount(parent) == 0)
        return;
}

void RemoteFileDialogPrivate::_q_fileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
    const RemoteFileDialog::FileMode fileMode = q_func()->fileMode();
    if (fileMode == RemoteFileDialog::Directory) {
        if (path == rootPath() && lineEdit()->text() == oldName)
            lineEdit()->setText(newName);
    }
}

void RemoteFileDialogPrivate::_q_directoryLoaded(const QString &path)
{
    Q_Q(RemoteFileDialog);

    if (currentHistoryLocation < 0) {
        qInfo() << "DIRECTORY loaded: no history<0, path = " << path;
        return;
    }
    if (currentHistoryLocation >= currentHistory.size()) {
        qInfo() << "DIRECTORY loaded: no history>>, path = " << path;
        return;
    }
    if (currentHistory[currentHistoryLocation] != path) {
        qInfo() << "DIRECTORY loaded: invalid history, path = " << path
                << "!=" << currentHistory[currentHistoryLocation];
        return;
    }

    qInfo() << "DIRECTORY updating" << path;
    updateCursor(false);

    QModelIndex root = model->setRootPath(path);
    if (root != rootIndex()) {
        if (path.endsWith(QLatin1Char('/')))
            completer->setCompletionPrefix(path);
        else
            completer->setCompletionPrefix(path + QLatin1Char('/'));
        setRootIndex(root);
    }
    qFileDialogUi->listView->selectionModel()->clear();

    qFileDialogUi->lookInCombo->populatePopup();
    _q_updateOkButton();

    auto pn = path;
    if (!pn.endsWith(QLatin1Char('/')))
        pn += QLatin1Char('/');
    pn += lineEdit()->text();
    QModelIndex idx = model->fsIndex(pn);
    qInfo() << "DIRECTORY w/ file" << pn << idx.isValid();
    if (!idx.isValid())
        return;
    select(idx);
}

void RemoteFileDialogPrivate::_q_emitUrlSelected(const QUrl &file)
{
    Q_Q(RemoteFileDialog);
    emit q->urlSelected(file);
    if (file.scheme() == QLatin1String("home")) {
        emit q->fileSelected(model->homePath().toString());
    } else if (file.isLocalFile()) {
        emit q->fileSelected(file.toLocalFile());
    }
}

void RemoteFileDialogPrivate::_q_emitUrlsSelected(const QList<QUrl> &files)
{
    Q_Q(RemoteFileDialog);
    emit q->urlsSelected(files);
    QStringList localFiles;
    for (const QUrl &file: files) {
        if (file.scheme() == QLatin1String("home")) {
            localFiles.append(model->homePath().toString());
        }
        if (file.isLocalFile()) {
            localFiles.append(file.toLocalFile());
        }
    }
    if (!localFiles.isEmpty())
        emit q->filesSelected(localFiles);
}

/*!
    \internal

    For the list and tree view watch keys to goto parent and back in the history

    returns \c true if handled
*/
bool RemoteFileDialogPrivate::itemViewKeyboardEvent(QKeyEvent *event)
{
#if QT_CONFIG(shortcut)
    Q_Q(RemoteFileDialog);
    if (event->matches(QKeySequence::Cancel)) {
        q->reject();
        return true;
    }
#endif
    switch (event->key()) {
    case Qt::Key_Backspace:
        _q_navigateToParent();
        return true;
    case Qt::Key_Back:
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled())
            return false;
#endif
    case Qt::Key_Left:
        if (event->key() == Qt::Key_Back || event->modifiers() == Qt::AltModifier) {
            _q_navigateBackward();
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

void RemoteFileDialogComboBox::setFileDialogPrivate(RemoteFileDialogPrivate *d_pointer)
{
    d_ptr = d_pointer;
    urlModel = new RemoteUrlModel(this);
    urlModel->showFullPath = true;
    urlModel->setFileSystemModel(d_ptr->model);
    setModel(urlModel);
}

void RemoteFileDialogComboBox::populatePopup()
{
    urlModel->setUrls(QList<QUrl>());
    QList<QUrl> list;
    QModelIndex idx = d_ptr->model->fsIndex(d_ptr->rootPath());
    while (idx.isValid()) {
        QUrl url = QUrl::fromLocalFile(idx.data(AbstractFileSystemModel::FilePathRole).toString());
        qInfo() << "history url" << url;
        if (url.isValid())
            list.append(url);
        idx = idx.parent();
    }
    // add "my computer"
    list.append(QUrl(QLatin1String("file:")));
    urlModel->addUrls(list, 0);
    idx = model()->index(model()->rowCount() - 1, 0);

    // append history
    QList<QUrl> urls;
    for (int i = 0; i < m_history.count(); ++i) {
        QUrl path = QUrl::fromLocalFile(m_history.at(i));
        if (!urls.contains(path))
            urls.prepend(path);
    }
    if (urls.count() > 0) {
        model()->insertRow(model()->rowCount());
        idx = model()->index(model()->rowCount() - 1, 0);
        // ### TODO maybe add a horizontal line before this
        model()->setData(idx, RemoteFileDialog::tr("Recent Places"));
        QStandardItemModel *m = qobject_cast<QStandardItemModel *>(model());
        if (m) {
            Qt::ItemFlags flags = m->flags(idx);
            flags &= ~Qt::ItemIsEnabled;
            m->item(idx.row(), idx.column())->setFlags(flags);
        }
        urlModel->addUrls(urls, -1, false);
    }
    setCurrentIndex(0);
}

void RemoteFileDialogComboBox::showPopup()
{
#if 1
    if (model()->rowCount() <= 1) {
        populatePopup();
    }
#endif
    QComboBox::showPopup();
}


// Exact same as QComboBox::paintEvent(), except we elide the text.
void RemoteFileDialogComboBox::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    // draw the combobox frame, focusrect and selected etc.
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QRect editRect = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);
    int size = editRect.width() - opt.iconSize.width() - 4;
    opt.currentText = opt.fontMetrics.elidedText(opt.currentText, Qt::ElideMiddle, size);
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // draw the icon and text
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

RemoteFileDialogListView::RemoteFileDialogListView(QWidget *parent): QListView(parent)
{}

void RemoteFileDialogListView::setFileDialogPrivate(RemoteFileDialogPrivate *d_pointer)
{
    d_ptr = d_pointer;
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setWrapping(true);
    setResizeMode(QListView::Adjust);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);
#ifndef QT_NO_DRAGANDDROP
    setDragDropMode(QAbstractItemView::InternalMove);
#endif
}

QSize RemoteFileDialogListView::sizeHint() const
{
    int height = qMax(10, sizeHintForRow(0));
    return QSize(QListView::sizeHint().width() * 2, height * 30);
}

void RemoteFileDialogListView::keyPressEvent(QKeyEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
        QListView::keyPressEvent(e);
        return;
    }
#endif // QT_KEYPAD_NAVIGATION

    if (!d_ptr->itemViewKeyboardEvent(e))
        QListView::keyPressEvent(e);
    e->accept();
}

RemoteFileDialogTreeView::RemoteFileDialogTreeView(QWidget *parent): QTreeView(parent)
{}

void RemoteFileDialogTreeView::setFileDialogPrivate(RemoteFileDialogPrivate *d_pointer)
{
    d_ptr = d_pointer;
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setSortingEnabled(true);
    header()->setSortIndicator(0, Qt::AscendingOrder);
    header()->setStretchLastSection(false);
    setTextElideMode(Qt::ElideMiddle);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);
#ifndef QT_NO_DRAGANDDROP
    setDragDropMode(QAbstractItemView::InternalMove);
#endif
}

void RemoteFileDialogTreeView::keyPressEvent(QKeyEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
        QTreeView::keyPressEvent(e);
        return;
    }
#endif // QT_KEYPAD_NAVIGATION

    if (!d_ptr->itemViewKeyboardEvent(e))
        QTreeView::keyPressEvent(e);
    e->accept();
}

QSize RemoteFileDialogTreeView::sizeHint() const
{
    int height = qMax(10, sizeHintForRow(0));
    QSize sizeHint = header()->sizeHint();
    return QSize(sizeHint.width() * 4, height * 30);
}

/*!
    // FIXME: this is a hack to avoid propagating key press events
    // to the dialog and from there to the "Ok" button
*/
void RemoteFileDialogLineEdit::keyPressEvent(QKeyEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
        QLineEdit::keyPressEvent(e);
        return;
    }
#endif // QT_KEYPAD_NAVIGATION

#if QT_CONFIG(shortcut)
    int key = e->key();
#endif
    QKeyEvent *syn = nullptr;
#if QT_CONFIG(completer)
    if (e->modifiers() == Qt::ControlModifier && completer() && completer()->popup() &&
        completer()->popup()->isVisible()) {
        if (e->key() == Qt::Key_N) {
            syn = new QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::ControlModifier);
        }
        if (e->key() == Qt::Key_P) {
            syn = new QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::ControlModifier);
        }
    }
#endif
    QLineEdit::keyPressEvent(syn ? syn : e);
#if QT_CONFIG(shortcut)
    if (!e->matches(QKeySequence::Cancel) && key != Qt::Key_Back)
#endif
        e->accept();
    if (syn)
        e->accept();
    delete syn;
}


QString RemoteFSCompleter::pathFromIndex(const QModelIndex &index) const
{
    const AbstractFileSystemModel *dirModel;
    if (proxyModel)
        dirModel = qobject_cast<const AbstractFileSystemModel *>(proxyModel->sourceModel());
    else
        dirModel = sourceModel;
    QString currentLocation = dirModel->rootPath();
    QString path = index.data(AbstractFileSystemModel::FilePathRole).toString();
    if (!currentLocation.isEmpty() && path.startsWith(currentLocation)) {
#if defined(Q_OS_UNIX)
        if (currentLocation == QDir::separator())
            return path.mid(currentLocation.length());
#endif
        if (currentLocation.endsWith(QLatin1Char('/')))
            return path.mid(currentLocation.length());
        else
            return path.mid(currentLocation.length() + 1);
    }
    return index.data(AbstractFileSystemModel::FilePathRole).toString();
}

QStringList RemoteFSCompleter::splitPath(const QString &path) const
{
    if (path.isEmpty())
        return QStringList(completionPrefix());

    AbstractFileSystemModel *dirModel;
    if (proxyModel)
        dirModel = qobject_cast<AbstractFileSystemModel *>(proxyModel->sourceModel());
    else
        dirModel = sourceModel;

    QString pathCopy = QDir::toNativeSeparators(path);
    QString sep = QDir::separator();
    QString doubleSlash(QLatin1String("\\\\"));
    if (dirModel->isWindows()) {
        if (pathCopy == QLatin1String("\\") || pathCopy == QLatin1String("\\\\"))
            return QStringList(pathCopy);
        if (pathCopy.startsWith(doubleSlash))
            pathCopy = pathCopy.mid(2);
        else
            doubleSlash.clear();
    } else {
        QString tildeExpanded = qt_tildeExpansion(dirModel, pathCopy);
        if (tildeExpanded != pathCopy) {
            dirModel->fetchMore(dirModel->fsIndex(tildeExpanded));
        }
        pathCopy = std::move(tildeExpanded);
    }

    QRegExp re(QLatin1Char('[') + QRegExp::escape(sep) + QLatin1Char(']'));

    bool startsFromRoot = false;
    QStringList parts;
    if (dirModel->isWindows()) {
        parts = pathCopy.split(re, QString::SkipEmptyParts);
        if (!doubleSlash.isEmpty() && !parts.isEmpty())
            parts[0].prepend(doubleSlash);
        if (pathCopy.endsWith(sep))
            parts.append(QString());
        startsFromRoot = !parts.isEmpty() && parts[0].endsWith(QLatin1Char(':'));
    } else {
        parts = pathCopy.split(re);
        if (pathCopy[0] == sep[0]) // read the "/" at the beginning as the split removed it
            parts[0] = sep[0];
        startsFromRoot = pathCopy[0] == sep[0];
    }

    if (parts.count() == 1 || (parts.count() > 1 && !startsFromRoot)) {
        QString currentLocation = QDir::toNativeSeparators(dirModel->rootPath());
        if (dirModel->isWindows()) {
            if (currentLocation.endsWith(QLatin1Char(':')))
                currentLocation.append(sep);
        }
        if (currentLocation.contains(sep) && path != currentLocation) {
            QStringList currentLocationList = splitPath(currentLocation);
            while (!currentLocationList.isEmpty() && parts.count() > 0 && parts.at(0) == QLatin1String("..")) {
                parts.removeFirst();
                currentLocationList.removeLast();
            }
            if (!currentLocationList.isEmpty() && currentLocationList.constLast().isEmpty())
                currentLocationList.removeLast();
            return currentLocationList + parts;
        }
    }
    return parts;
}

QT_END_NAMESPACE

#include "moc_remotefiledialog.cpp"
