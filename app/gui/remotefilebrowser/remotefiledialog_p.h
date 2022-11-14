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

#ifndef REMOTEFILEDIALOG_P_H
#define REMOTEFILEDIALOG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

//#include <QtWidgets/private/qtwidgetsglobal_p.h>

#include "remotefiledialog.h"
//#include "qdialog_p.h"
//#include "private/qdialog_p.h"
#include "qplatformdefs.h"

//#include "qfilesystemmodel_p.h"
#include "abstractfilesystemmodel.h"
#include <qlistview.h>
#include <qtreeview.h>
#include <qcombobox.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qurl.h>
#include <qstackedwidget.h>
#include <qdialogbuttonbox.h>
#include <qabstractproxymodel.h>
#if QT_CONFIG(completer)
#include <qcompleter.h>
#endif
#include <qpointer.h>
#include <qdebug.h>
#include "qsidebar_p.h"
#include "remotefscompleter_p.h"

#include "vistlefilesystemmodel.h"

#if defined(Q_OS_UNIX)
#include <unistd.h>
#endif

QT_REQUIRE_CONFIG(filedialog);

QT_BEGIN_NAMESPACE

class RemoteFileDialogListView;
class RemoteFileDialogTreeView;
class RemoteFileDialogLineEdit;
class QGridLayout;
class QCompleter;
class QHBoxLayout;
class Ui_RemoteFileDialog;

struct RemoteFileDialogArgs {
    RemoteFileDialogArgs(): parent(0), mode(RemoteFileDialog::AnyFile) {}

    QWidget *parent;
    QString caption;
    QUrl directory;
    QString selection;
    QString filter;
    RemoteFileDialog::FileMode mode;
    RemoteFileDialog::Options options;
};

#define UrlRole (Qt::UserRole + 1)

class RemoteFileDialogPrivate // : public QDialogPrivate
{
    RemoteFileDialog *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(RemoteFileDialog)

public:
    RemoteFileDialogPrivate(RemoteFileDialog *);

    void createToolButtons();
    void createMenuActions();
    void createWidgets();

    void init(const QUrl &directory = QUrl(), const QString &nameFilter = QString(),
              const QString &caption = QString());
    bool itemViewKeyboardEvent(QKeyEvent *event);
    static QUrl workingDirectory(const QUrl &path);
    static QString initialSelection(const QUrl &path);
    QStringList typedFiles() const;
    QList<QUrl> userSelectedFiles() const;
    QStringList addDefaultSuffixToFiles(const QStringList &filesToFix) const;
    QList<QUrl> addDefaultSuffixToUrls(const QList<QUrl> &urlsToFix) const;
    bool removeDirectory(const QString &path);
    void setLabelTextControl(RemoteFileDialog::DialogLabel label, const QString &text);
    inline void updateLookInLabel();
    inline void updateFileNameLabel();
    inline void updateFileTypeLabel();
    void updateOkButtonText(bool saveAsOnFolder = false);
    void updateCancelButtonText();

    inline QModelIndex mapToSource(const QModelIndex &index) const;
    inline QModelIndex mapFromSource(const QModelIndex &index) const;
    inline QModelIndex rootIndex() const;
    inline void setRootIndex(const QModelIndex &index) const;
    inline QModelIndex select(const QModelIndex &index) const;
    inline QString rootPath() const;

    QLineEdit *lineEdit() const;

    static int maxNameLength(const QString &path);

    QString basename(const QString &path) const
    {
        int separator = QDir::toNativeSeparators(path).lastIndexOf(QDir::separator());
        if (separator != -1)
            return path.mid(separator + 1);
        return path;
    }

    QDir::Filters filterForMode(QDir::Filters filters) const
    {
        filters |= QDir::Drives | QDir::AllDirs | QDir::Files | QDir::Dirs;
        return filters;
    }

    QAbstractItemView *currentView() const;

    static inline QString toInternal(const QString &path)
    {
#if defined(Q_OS_WIN)
        QString n(path);
        n.replace(QLatin1Char('\\'), QLatin1Char('/'));
        return n;
#else // the compile should optimize away this
        return path;
#endif
    }

#ifndef QT_NO_SETTINGS
    void saveSettings();
    bool restoreFromSettings();
#endif

    bool restoreWidgetState(QStringList &history, int splitterPosition);
    static void setLastVisitedDirectory(const QUrl &dir);
    void retranslateWindowTitle();
    void retranslateStrings();
    void emitFilesSelected(const QStringList &files);
    void updateCursor(bool waiting);

    void _q_goHome();
    void _q_pathChanged(const QString &);
    void _q_navigateBackward();
    void _q_navigateForward();
    void _q_navigateToParent();
    void _q_createDirectory();
    void _q_showListView();
    void _q_showDetailsView();
    void _q_showContextMenu(const QPoint &position);
    void _q_renameCurrent();
    void _q_deleteCurrent();
    void _q_showHidden();
    void _q_showHeader(QAction *);
    void _q_updateOkButton();
    void _q_currentChanged(const QModelIndex &index);
    void _q_enterDirectory(const QModelIndex &index);
    void _q_emitUrlSelected(const QUrl &file);
    void _q_emitUrlsSelected(const QList<QUrl> &files);
    void _q_goToDirectory(const QString &);
    void _q_useNameFilter(int index);
    void _q_selectionChanged();
    void _q_goToUrl(const QUrl &url);
    void _q_autoCompleteFileName(const QString &);
    void _q_rowsInserted(const QModelIndex &parent);
    void _q_fileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void _q_directoryLoaded(const QString &path);

    // layout
#ifndef QT_NO_PROXYMODEL
    QAbstractProxyModel *proxyModel;
#endif

    // data
    QStringList watching;
    AbstractFileSystemModel *model;

    RemoteFSCompleter *completer;

    QString setWindowTitle;

    QStringList currentHistory;
    int currentHistoryLocation;

    QAction *showHiddenAction;
    QAction *newFolderAction;

    bool useDefaultCaption;

    // setVisible_sys returns true if it ends up showing a native
    // dialog. Returning false means that a non-native dialog must be
    // used instead.
    inline bool usingWidgets() const;

    //////////////////////////////////////////////

    QScopedPointer<Ui_RemoteFileDialog> qFileDialogUi;

    QString acceptLabel;

    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;
    QByteArray signalToDisconnectOnClose;

    QSharedPointer<RemoteFileDialogOptions> options;

    // Memory of what was read from QSettings in restoreState() in case widgets are not used
    QByteArray splitterState;
    QByteArray headerData;
    QList<QUrl> sidebarUrls;

    ~RemoteFileDialogPrivate();

    Q_DISABLE_COPY(RemoteFileDialogPrivate)
};

class RemoteFileDialogLineEdit: public QLineEdit {
public:
    RemoteFileDialogLineEdit(QWidget *parent = 0): QLineEdit(parent), d_ptr(0) {}
    void setFileDialogPrivate(RemoteFileDialogPrivate *d_pointer) { d_ptr = d_pointer; }
    void keyPressEvent(QKeyEvent *e) override;
    bool hideOnEsc;

private:
    RemoteFileDialogPrivate *d_ptr;
};

class RemoteFileDialogComboBox: public QComboBox {
public:
    RemoteFileDialogComboBox(QWidget *parent = 0): QComboBox(parent), urlModel(0) {}
    void setFileDialogPrivate(RemoteFileDialogPrivate *d_pointer);
    void showPopup() override;
    void setHistory(const QStringList &paths);
    void populatePopup();
    QStringList history() const { return m_history; }
    void paintEvent(QPaintEvent *) override;

private:
    RemoteUrlModel *urlModel;
    RemoteFileDialogPrivate *d_ptr;
    QStringList m_history;
};

class RemoteFileDialogListView: public QListView {
public:
    RemoteFileDialogListView(QWidget *parent = 0);
    void setFileDialogPrivate(RemoteFileDialogPrivate *d_pointer);
    QSize sizeHint() const override;

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    RemoteFileDialogPrivate *d_ptr;
};

class RemoteFileDialogTreeView: public QTreeView {
public:
    RemoteFileDialogTreeView(QWidget *parent);
    void setFileDialogPrivate(RemoteFileDialogPrivate *d_pointer);
    QSize sizeHint() const override;

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    RemoteFileDialogPrivate *d_ptr;
};

QModelIndex RemoteFileDialogPrivate::mapToSource(const QModelIndex &index) const
{
#ifdef QT_NO_PROXYMODEL
    return index;
#else
    return proxyModel ? proxyModel->mapToSource(index) : index;
#endif
}
QModelIndex RemoteFileDialogPrivate::mapFromSource(const QModelIndex &index) const
{
#ifdef QT_NO_PROXYMODEL
    return index;
#else
    return proxyModel ? proxyModel->mapFromSource(index) : index;
#endif
}

QString RemoteFileDialogPrivate::rootPath() const
{
    return (model ? model->rootPath() : QStringLiteral("/"));
}


QT_END_NAMESPACE

#endif // QFILEDIALOG_P_H
