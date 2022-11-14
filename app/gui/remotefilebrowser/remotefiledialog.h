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

#ifndef REMOTEFILEDIALOG_H
#define REMOTEFILEDIALOG_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qdir.h>
#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <QtWidgets/qdialog.h>

#include <QtCore/qglobal.h>

QT_REQUIRE_CONFIG(filedialog);

QT_BEGIN_NAMESPACE

class QModelIndex;
class QItemSelection;
struct RemoteFileDialogArgs;
class RemoteFileIconProvider;
class RemoteFileDialogPrivate;
class QAbstractItemDelegate;
class QAbstractProxyModel;
class AbstractFileInfoGatherer;
class AbstractFileSystemModel;

class RemoteFileDialog: public QDialog {
    Q_OBJECT
    Q_PROPERTY(ViewMode viewMode READ viewMode WRITE setViewMode)
    Q_PROPERTY(FileMode fileMode READ fileMode WRITE setFileMode)
    Q_PROPERTY(AcceptMode acceptMode READ acceptMode WRITE setAcceptMode)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE false)
    Q_PROPERTY(bool resolveSymlinks READ resolveSymlinks WRITE setResolveSymlinks DESIGNABLE false)
    Q_PROPERTY(bool confirmOverwrite READ confirmOverwrite WRITE setConfirmOverwrite DESIGNABLE false)
    Q_PROPERTY(QString defaultSuffix READ defaultSuffix WRITE setDefaultSuffix)
    Q_PROPERTY(bool nameFilterDetailsVisible READ isNameFilterDetailsVisible WRITE setNameFilterDetailsVisible
                   DESIGNABLE false)
    Q_PROPERTY(Options options READ options WRITE setOptions)
    Q_PROPERTY(QStringList supportedSchemes READ supportedSchemes WRITE setSupportedSchemes)

public:
    enum ViewMode { Detail, List };
    Q_ENUM(ViewMode)
    enum FileMode { AnyFile, ExistingFile, Directory, ExistingFiles };
    Q_ENUM(FileMode)
    enum AcceptMode { AcceptOpen, AcceptSave };
    Q_ENUM(AcceptMode)
    enum DialogLabel { LookIn, FileName, FileType, Accept, Reject };

    enum Option {
        ShowDirsOnly = 0x00000001,
        DontResolveSymlinks = 0x00000002,
        DontConfirmOverwrite = 0x00000004,
        DontUseSheet = 0x00000008,
        DontUseNativeDialog = 0x00000010, // default, not changeable
        ReadOnly = 0x00000020,
        HideNameFilterDetails = 0x00000040,
        DontUseCustomDirectoryIcons = 0x00000080 // default, not changeable
    };
    Q_ENUM(Option)
    Q_DECLARE_FLAGS(Options, Option)
    Q_FLAG(Options)

    RemoteFileDialog(AbstractFileSystemModel *model, QWidget *parent, Qt::WindowFlags f);
    RemoteFileDialog(AbstractFileSystemModel *model, QWidget *parent = nullptr, const QString &caption = QString(),
                     const QString &directory = QString(), const QString &filter = QString());
    ~RemoteFileDialog();

    void setDirectory(const QString &directory);
    QString directory() const;

    void setDirectoryUrl(const QUrl &directory);
    QUrl directoryUrl() const;

    void selectFile(const QString &filename);
    QStringList selectedFiles() const;

    void selectUrl(const QUrl &url);
    QList<QUrl> selectedUrls() const;

    void setNameFilterDetailsVisible(bool enabled);
    bool isNameFilterDetailsVisible() const;

    void setNameFilter(const QString &filter);
    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const;
    void selectNameFilter(const QString &filter);
    QString selectedMimeTypeFilter() const;
    QString selectedNameFilter() const;

#ifndef QT_NO_MIMETYPE
    void setMimeTypeFilters(const QStringList &filters);
    QStringList mimeTypeFilters() const;
    void selectMimeTypeFilter(const QString &filter);
#endif

    QDir::Filters filter() const;
    void setFilter(QDir::Filters filters);

    void setViewMode(ViewMode mode);
    ViewMode viewMode() const;

    void setFileMode(FileMode mode);
    FileMode fileMode() const;

    void setAcceptMode(AcceptMode mode);
    AcceptMode acceptMode() const;

    void setReadOnly(bool enabled);
    bool isReadOnly() const;

    void setResolveSymlinks(bool enabled);
    bool resolveSymlinks() const;

    void setSidebarUrls(const QList<QUrl> &urls);
    QList<QUrl> sidebarUrls() const;

    void setConfirmOverwrite(bool enabled);
    bool confirmOverwrite() const;

    void setDefaultSuffix(const QString &suffix);
    QString defaultSuffix() const;

    void setHistory(const QStringList &paths);
    QStringList history() const;

    void setItemDelegate(QAbstractItemDelegate *delegate);
    QAbstractItemDelegate *itemDelegate() const;

    void setIconProvider(RemoteFileIconProvider *provider);
    RemoteFileIconProvider *iconProvider() const;

    void setLabelText(DialogLabel label, const QString &text);
    QString labelText(DialogLabel label) const;

    void setSupportedSchemes(const QStringList &schemes);
    QStringList supportedSchemes() const;

#ifndef QT_NO_PROXYMODEL
    void setProxyModel(QAbstractProxyModel *model);
    QAbstractProxyModel *proxyModel() const;
#endif

    void setOption(Option option, bool on = true);
    bool testOption(Option option) const;
    void setOptions(Options options);
    Options options() const;

    using QDialog::open;
    void open(QObject *receiver, const char *member);
    void setVisible(bool visible) override;

Q_SIGNALS:
    void fileSelected(const QString &file);
    void filesSelected(const QStringList &files);
    void currentChanged(const QString &path);
    void directoryEntered(const QString &directory);

    void urlSelected(const QUrl &url);
    void urlsSelected(const QList<QUrl> &urls);
    void currentUrlChanged(const QUrl &url);
    void directoryUrlEntered(const QUrl &directory);

    void filterSelected(const QString &filter);

public:
protected:
    RemoteFileDialog(const RemoteFileDialogArgs &args);
    void done(int result) override;
    void accept() override;
    void changeEvent(QEvent *e) override;

private:
    Q_DECLARE_PRIVATE_D(vd_ptr, RemoteFileDialog)
    Q_DISABLE_COPY(RemoteFileDialog)

    Q_PRIVATE_SLOT(d_func(), void _q_pathChanged(const QString &))
    Q_PRIVATE_SLOT(d_func(), void _q_directoryLoaded(const QString &))

    Q_PRIVATE_SLOT(d_func(), void _q_navigateBackward())
    Q_PRIVATE_SLOT(d_func(), void _q_navigateForward())
    Q_PRIVATE_SLOT(d_func(), void _q_navigateToParent())
    Q_PRIVATE_SLOT(d_func(), void _q_createDirectory())
    Q_PRIVATE_SLOT(d_func(), void _q_showListView())
    Q_PRIVATE_SLOT(d_func(), void _q_showDetailsView())
    Q_PRIVATE_SLOT(d_func(), void _q_showContextMenu(const QPoint &))
    Q_PRIVATE_SLOT(d_func(), void _q_showHidden())
    Q_PRIVATE_SLOT(d_func(), void _q_updateOkButton())
    Q_PRIVATE_SLOT(d_func(), void _q_currentChanged(const QModelIndex &index))
    Q_PRIVATE_SLOT(d_func(), void _q_enterDirectory(const QModelIndex &index))
    Q_PRIVATE_SLOT(d_func(), void _q_emitUrlSelected(const QUrl &))
    Q_PRIVATE_SLOT(d_func(), void _q_emitUrlsSelected(const QList<QUrl> &))
    Q_PRIVATE_SLOT(d_func(), void _q_goToDirectory(const QString &path))
    Q_PRIVATE_SLOT(d_func(), void _q_useNameFilter(int index))
    Q_PRIVATE_SLOT(d_func(), void _q_selectionChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_goToUrl(const QUrl &url))
    Q_PRIVATE_SLOT(d_func(), void _q_goHome())
    Q_PRIVATE_SLOT(d_func(), void _q_showHeader(QAction *))
    Q_PRIVATE_SLOT(d_func(), void _q_autoCompleteFileName(const QString &text))
    Q_PRIVATE_SLOT(d_func(), void _q_rowsInserted(const QModelIndex &parent))
    Q_PRIVATE_SLOT(d_func(), void _q_fileRenamed(const QString &path, const QString &oldName, const QString &newName))
    QString fileFromPath(const QString &rootPath, QString path);
    friend class QPlatformDialogHelper;
    QScopedPointer<RemoteFileDialogPrivate> vd_ptr;
    AbstractFileSystemModel *m_model = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RemoteFileDialog::Options)

QT_END_NAMESPACE

#endif // QFILEDIALOG_H
