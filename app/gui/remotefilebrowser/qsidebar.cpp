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

#include "qsidebar_p.h"

#include <qaction.h>
#include <qurl.h>
#if QT_CONFIG(menu)
#include <qmenu.h>
#endif
#include <qmimedata.h>
#include <qevent.h>
#include "remotefileiconprovider.h"
#include "remotefiledialog.h"
#include "abstractfilesystemmodel.h"

QT_BEGIN_NAMESPACE

const QLatin1String HomeScheme("home");
const QUrl HomeUrl(HomeScheme + QLatin1String(":"));

void RFBSideBarDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    QVariant value = index.data(RemoteUrlModel::EnabledRole);
    if (value.isValid()) {
        //If the bookmark/entry is not enabled then we paint it in gray
        if (!qvariant_cast<bool>(value))
            option->state &= ~QStyle::State_Enabled;
    }
}

/*!
    \internal
    \class RemoteUrlModel
    RemoteUrlModel lets you have indexes from a AbstractFileSystemModel to a list.  When AbstractFileSystemModel
    changes them RemoteUrlModel will automatically update.

    Example usage: File dialog sidebar and combo box
 */
RemoteUrlModel::RemoteUrlModel(QObject *parent): QStandardItemModel(parent), showFullPath(false), fileSystemModel(0)
{}

/*!
    \reimp
*/
QStringList RemoteUrlModel::mimeTypes() const
{
    return QStringList(QLatin1String("text/uri-list"));
}

/*!
    \reimp
*/
Qt::ItemFlags RemoteUrlModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QStandardItemModel::flags(index);
    if (index.isValid()) {
        flags &= ~Qt::ItemIsEditable;
        // ### some future version could support "moving" urls onto a folder
        flags &= ~Qt::ItemIsDropEnabled;
    }

    if (index.data(Qt::DecorationRole).isNull())
        flags &= ~Qt::ItemIsEnabled;

    return flags;
}

/*!
    \reimp
*/
QMimeData *RemoteUrlModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> list;
    for (const auto &index: indexes) {
        if (index.column() == 0)
            list.append(index.data(UrlRole).toUrl());
    }
    QMimeData *data = new QMimeData();
    data->setUrls(list);
    return data;
}

#ifndef QT_NO_DRAGANDDROP

/*!
    Decide based upon the data if it should be accepted or not

    We only accept dirs and not files
*/
bool RemoteUrlModel::canDrop(QDragEnterEvent *event)
{
    if (!event->mimeData()->formats().contains(mimeTypes().constFirst()))
        return false;

    const QList<QUrl> list = event->mimeData()->urls();
    for (const auto &url: list) {
        const QModelIndex idx = fileSystemModel->fsIndex(url.toLocalFile());
        if (!fileSystemModel->isDir(idx))
            return false;
    }
    return true;
}

/*!
    \reimp
*/
bool RemoteUrlModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                                  const QModelIndex &parent)
{
    if (!data->formats().contains(mimeTypes().constFirst()))
        return false;
    Q_UNUSED(action);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    addUrls(data->urls(), row);
    return true;
}

#endif // QT_NO_DRAGANDDROP

/*!
    \reimp

    If the role is the UrlRole then handle otherwise just pass to QStandardItemModel
*/
bool RemoteUrlModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (value.type() == QVariant::Url) {
        QUrl url = value.toUrl();
        QString path;
        if (url.scheme() == HomeScheme) {
            path = fileSystemModel->homePath().toString();
            if (showFullPath) {
                QStandardItemModel::setData(
                    index, fileSystemModel->homePath(AbstractFileSystemModel::FilePathRole).toString());
            } else {
                QStandardItemModel::setData(index,
                                            fileSystemModel->homePath(AbstractFileSystemModel::FilePathRole).toString(),
                                            Qt::ToolTipRole);
                QStandardItemModel::setData(index, fileSystemModel->homePath().toString());
            }
            QStandardItemModel::setData(index, fileSystemModel->homePath(Qt::DecorationRole), Qt::DecorationRole);
        } else {
            path = url.toLocalFile();
            QModelIndex dirIndex = fileSystemModel->fsIndex(path);
            //On windows the popup display the "C:\", convert to nativeSeparators
            if (showFullPath) {
                QStandardItemModel::setData(
                    index, fileSystemModel->data(dirIndex, AbstractFileSystemModel::FilePathRole).toString());
            } else {
                QStandardItemModel::setData(
                    index, fileSystemModel->data(dirIndex, AbstractFileSystemModel::FilePathRole).toString(),
                    Qt::ToolTipRole);
                QStandardItemModel::setData(index, fileSystemModel->data(dirIndex).toString());
            }
            QStandardItemModel::setData(index, fileSystemModel->data(dirIndex, Qt::DecorationRole), Qt::DecorationRole);
        }
        QStandardItemModel::setData(index, url, UrlRole);
        return true;
    }
    return QStandardItemModel::setData(index, value, role);
}

void RemoteUrlModel::setUrl(const QModelIndex &index, const QUrl &url, const QModelIndex &dirIndex)
{
    setData(index, url, UrlRole);
    if (url.scheme() == HomeScheme) {
        QString newName;
        if (showFullPath) {
            //On windows the popup display the "C:\", convert to nativeSeparators
            newName = fileSystemModel->homePath(AbstractFileSystemModel::FilePathRole).toString();
        } else {
            newName = fileSystemModel->homePath().toString();
        }
        if (index.data().toString() != newName)
            setData(index, newName);
        setData(index, fileSystemModel->homePath(Qt::DecorationRole), Qt::DecorationRole);
        setData(index, true, EnabledRole);
    } else if (url.path().isEmpty()) {
        setData(index, QUrl("file:"), UrlRole);
        setData(index, fileSystemModel->myComputer());
        setData(index, fileSystemModel->myComputer(Qt::DecorationRole), Qt::DecorationRole);
        setData(index, true, EnabledRole);
    } else {
        QString newName;
        if (showFullPath) {
            //On windows the popup display the "C:\", convert to nativeSeparators
            newName = dirIndex.data(AbstractFileSystemModel::FilePathRole).toString();
        } else {
            newName = dirIndex.data().toString();
        }

        QIcon newIcon = qvariant_cast<QIcon>(dirIndex.data(Qt::DecorationRole));
#if 0
        if (!dirIndex.isValid()) {
            const RemoteFileIconProvider *provider = fileSystemModel->iconProvider();
            if (provider)
                newIcon = provider->icon(RemoteFileIconProvider::Folder);
            newName = QFileInfo(url.toLocalFile()).fileName();
            if (!invalidUrls.contains(url))
                invalidUrls.append(url);
            //The bookmark is invalid then we set to false the EnabledRole
            setData(index, false, EnabledRole);
        } else {
            //The bookmark is valid then we set to true the EnabledRole
            setData(index, true, EnabledRole);
        }
#endif
        setData(index, true, EnabledRole);

        // Make sure that we have at least 32x32 images
        const QSize size = newIcon.actualSize(QSize(32, 32));
        if (size.width() < 32) {
            QPixmap smallPixmap = newIcon.pixmap(QSize(32, 32));
            newIcon.addPixmap(smallPixmap.scaledToWidth(32, Qt::SmoothTransformation));
        }

        if (index.data().toString() != newName)
            setData(index, newName);
        QIcon oldIcon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (oldIcon.cacheKey() != newIcon.cacheKey())
            setData(index, newIcon, Qt::DecorationRole);
    }
}

void RemoteUrlModel::setUrls(const QList<QUrl> &list)
{
    removeRows(0, rowCount());
    invalidUrls.clear();
    watching.clear();
    addUrls(list, 0);
}

/*!
    Add urls \a list into the list at \a row.  If move then movie
    existing ones to row.

    \sa dropMimeData()
*/
void RemoteUrlModel::addUrls(const QList<QUrl> &list, int row, bool move)
{
    if (row == -1)
        row = rowCount();
    row = qMin(row, rowCount());

    Qt::CaseSensitivity cs = Qt::CaseSensitive;
    if (fileSystemModel->isWindows()) {
        cs = Qt::CaseInsensitive;
    }
    const QString home = fileSystemModel->homePath().toString();

    for (int i = list.count() - 1; i >= 0; --i) {
        QUrl url = list.at(i);
        if (!url.isValid())
            continue;
        if (url.scheme() != QLatin1String("file") && url.scheme() != HomeScheme)
            continue;
        QModelIndex idx;
        QString cleanUrl;
        if (url.scheme() == HomeScheme) {
            cleanUrl = home;
        } else {
            //this makes sure the url is clean
            cleanUrl = QDir::cleanPath(url.toLocalFile());
            if (!home.isEmpty() && !cleanUrl.compare(home, cs)) {
                url = HomeUrl;
            } else if (!cleanUrl.isEmpty()) {
                url = QUrl::fromLocalFile(cleanUrl);
            }

            idx = fileSystemModel->fsIndex(cleanUrl);
            if (!fileSystemModel->isDir(idx))
                continue;
            watching.append({idx, cleanUrl});
        }

        for (int j = 0; move && j < rowCount(); ++j) {
            QUrl localUrl = index(j, 0).data(UrlRole).toUrl();
            QString local = localUrl.toLocalFile();
            if (localUrl.scheme() == HomeScheme) {
                local = home;
            }
            if (home.isEmpty() && localUrl.scheme() != url.scheme() &&
                (localUrl.scheme() == HomeScheme || url.scheme() == HomeScheme)) {
                continue;
            }
            if (!cleanUrl.compare(local, cs)) {
                removeRow(j);
                if (j <= row)
                    row--;
                break;
            }
        }

        row = qMax(row, 0);
        insertRows(row, 1);
        setUrl(index(row, 0), url, idx);
    }
}

/*!
    Return the complete list of urls in a QList.
*/
QList<QUrl> RemoteUrlModel::urls() const
{
    QList<QUrl> list;
    const int numRows = rowCount();
    list.reserve(numRows);
    for (int i = 0; i < numRows; ++i)
        list.append(data(index(i, 0), UrlRole).toUrl());
    return list;
}

/*!
    AbstractFileSystemModel to get index's from, clears existing rows
*/
void RemoteUrlModel::setFileSystemModel(AbstractFileSystemModel *model)
{
    if (model == fileSystemModel)
        return;
    if (fileSystemModel != 0) {
        disconnect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this,
                   SLOT(dataChanged(QModelIndex, QModelIndex)));
        disconnect(model, SIGNAL(layoutChanged()), this, SLOT(layoutChanged()));
        disconnect(model, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(layoutChanged()));
        disconnect(model, SIGNAL(initialized()), this, SLOT(updateHomePath()));
    }
    fileSystemModel = model;
    if (fileSystemModel != 0) {
        connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this,
                SLOT(dataChanged(QModelIndex, QModelIndex)));
        connect(model, SIGNAL(layoutChanged()), this, SLOT(layoutChanged()));
        connect(model, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(layoutChanged()));
        connect(model, SIGNAL(initialized()), this, SLOT(updateHomePath()));
    }
    clear();
    insertColumns(0, 1);
}

void RemoteUrlModel::updateHomePath()
{
    qInfo() << "UPDATING HOMEPATH";
    for (int j = 0; j < rowCount(); ++j) {
        auto idx = index(j, 0);
        QUrl url = idx.data(UrlRole).toUrl();
        if (url.scheme() == HomeScheme) {
            setData(idx, url);
            changed(fileSystemModel->homePath(AbstractFileSystemModel::FilePathRole).toString());
        }
    }
}

/*
    If one of the index's we are watching has changed update our internal data
*/
void RemoteUrlModel::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QModelIndex parent = topLeft.parent();
    for (int i = 0; i < watching.count(); ++i) {
        QModelIndex index = watching.at(i).index;
        if (index.model() && topLeft.model()) {
            Q_ASSERT(index.model() == topLeft.model());
        }
        if (index.row() >= topLeft.row() && index.row() <= bottomRight.row() && index.column() >= topLeft.column() &&
            index.column() <= bottomRight.column() && index.parent() == parent) {
            changed(watching.at(i).path);
        }
    }
}

/*!
    Re-get all of our data, anything could have changed!
 */
void RemoteUrlModel::layoutChanged()
{
    QStringList paths;
    const int numPaths = watching.count();
    paths.reserve(numPaths);
    for (int i = 0; i < numPaths; ++i)
        paths.append(watching.at(i).path);
    watching.clear();
    for (int i = 0; i < numPaths; ++i) {
        QString path = paths.at(i);
        QModelIndex newIndex = fileSystemModel->fsIndex(path);
        watching.append({newIndex, path});
        if (newIndex.isValid())
            changed(path);
    }
}

/*!
    The following path changed data update our copy of that data

    \sa layoutChanged(), dataChanged()
*/
void RemoteUrlModel::changed(const QString &path)
{
    for (int i = 0; i < rowCount(); ++i) {
        QModelIndex idx = index(i, 0);
        QUrl url = idx.data(UrlRole).toUrl();
        if (url.isLocalFile() && url.toLocalFile() == path) {
            setData(idx, idx.data(UrlRole).toUrl());
        }
    }
}

RFBSidebar::RFBSidebar(QWidget *parent): QListView(parent)
{}

void RFBSidebar::setModelAndUrls(AbstractFileSystemModel *model, const QList<QUrl> &newUrls)
{
    // ### TODO make icon size dynamic
    setIconSize(QSize(24, 24));
    setUniformItemSizes(true);
    urlModel = new RemoteUrlModel(this);
    urlModel->setFileSystemModel(model);
    setModel(urlModel);
    setItemDelegate(new RFBSideBarDelegate(this));

    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(clicked(QModelIndex)));
#ifndef QT_NO_DRAGANDDROP
    setDragDropMode(QAbstractItemView::DragDrop);
#endif
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    urlModel->setUrls(newUrls);
    setCurrentIndex(this->model()->index(0, 0));
}

RFBSidebar::~RFBSidebar()
{}

#ifndef QT_NO_DRAGANDDROP
void RFBSidebar::dragEnterEvent(QDragEnterEvent *event)
{
    if (urlModel->canDrop(event))
        QListView::dragEnterEvent(event);
}
#endif // QT_NO_DRAGANDDROP

QSize RFBSidebar::sizeHint() const
{
    if (model())
        return QListView::sizeHintForIndex(model()->index(0, 0)) + QSize(2 * frameWidth(), 2 * frameWidth());
    return QListView::sizeHint();
}

void RFBSidebar::selectUrl(const QUrl &url)
{
    disconnect(selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(clicked(QModelIndex)));

    const QString home = urlModel->fileSystemModel->homePath().toString();

    selectionModel()->clear();
    for (int i = 0; i < model()->rowCount(); ++i) {
        QUrl l = model()->index(i, 0).data(RemoteUrlModel::UrlRole).toUrl();
        if (l == url || (l.scheme() == HomeScheme && url.toLocalFile() == home)) {
            selectionModel()->select(model()->index(i, 0), QItemSelectionModel::Select);
            break;
        }
    }

    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(clicked(QModelIndex)));
}

#if QT_CONFIG(menu)
/*!
    \internal

    \sa removeEntry()
*/
void RFBSidebar::showContextMenu(const QPoint &position)
{
    QList<QAction *> actions;
    if (indexAt(position).isValid()) {
        QAction *action = new QAction(RemoteFileDialog::tr("Remove"), this);
        if (indexAt(position).data(RemoteUrlModel::UrlRole).toUrl().path().isEmpty())
            action->setEnabled(false);
        connect(action, SIGNAL(triggered()), this, SLOT(removeEntry()));
        actions.append(action);
    }
    if (actions.count() > 0)
        QMenu::exec(actions, mapToGlobal(position));
}
#endif // QT_CONFIG(menu)

/*!
    \internal

    \sa showContextMenu()
*/
void RFBSidebar::removeEntry()
{
    QList<QModelIndex> idxs = selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> indexes;
    const int numIndexes = idxs.count();
    indexes.reserve(numIndexes);
    for (int i = 0; i < numIndexes; i++)
        indexes.append(idxs.at(i));

    for (int i = 0; i < numIndexes; ++i) {
        if (!indexes.at(i).data(RemoteUrlModel::UrlRole).toUrl().path().isEmpty())
            model()->removeRow(indexes.at(i).row());
    }
}

/*!
    \internal

    \sa goToUrl()
*/
void RFBSidebar::clicked(const QModelIndex &index)
{
    QUrl url = model()->index(index.row(), 0).data(RemoteUrlModel::UrlRole).toUrl();
    emit goToUrl(url);
    selectUrl(url);
}

/*!
    \reimp
    Don't automatically select something
 */
void RFBSidebar::focusInEvent(QFocusEvent *event)
{
    QAbstractScrollArea::focusInEvent(event);
    viewport()->update();
}

/*!
    \reimp
 */
bool RFBSidebar::event(QEvent *event)
{
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = (QKeyEvent *)event;
        if (ke->key() == Qt::Key_Delete) {
            removeEntry();
            return true;
        }
    }
    return QListView::event(event);
}

QT_END_NAMESPACE

#include "moc_qsidebar_p.cpp"
