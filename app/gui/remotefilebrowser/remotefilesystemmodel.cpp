/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
*

QString RemoteFileSystemModel::workingDirectory() const
{

}* Licensees holding valid commercial Qt licenses may use this file in
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

#include "remotefilesystemmodel_p.h"
#include "remotefilesystemmodel.h"
#include <qlocale.h>
#include <qmimedata.h>
#include <qurl.h>
#include <qdebug.h>
#if QT_CONFIG(messagebox)
#include <qmessagebox.h>
#endif
#include <qapplication.h>
#include <qstyle.h>
#include <QtCore/qcollator.h>
#include <QRegularExpression>

#include <algorithm>

#ifdef Q_OS_WIN
#include <QtCore/QVarLengthArray>
#include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \enum RemoteFileSystemModel::Roles
    \value FileIconRole
    \value FilePathRole
    \value FileNameRole
    \value FilePermissions
*/

/*!
    \class RemoteFileSystemModel
    \since 4.4

    \brief The RemoteFileSystemModel class provides a data model for the local filesystem.

    \ingroup model-view
    \inmodule QtWidgets

    This class provides access to the local filesystem, providing functions
    for renaming and removing files and directories, and for creating new
    directories. In the simplest case, it can be used with a suitable display
    widget as part of a browser or filter.

    RemoteFileSystemModel can be accessed using the standard interface provided by
    QAbstractItemModel, but it also provides some convenience functions that are
    specific to a directory model.
    The fileInfo(), isDir(), fileName() and filePath() functions provide information
    about the underlying files and directories related to items in the model.
    Directories can be created and removed using mkdir(), rmdir().

    \note RemoteFileSystemModel requires an instance of \l QApplication.

    \section1 Example Usage

    A directory model that displays the contents of a default directory
    is usually constructed with a parent object:

    \snippet shareddirmodel/main.cpp 2

    A tree view can be used to display the contents of the model

    \snippet shareddirmodel/main.cpp 4

    and the contents of a particular directory can be displayed by
    setting the tree view's root index:

    \snippet shareddirmodel/main.cpp 7

    The view's root index can be used to control how much of a
    hierarchical model is displayed. RemoteFileSystemModel provides a convenience
    function that returns a suitable model index for a path to a
    directory within the model.

    \section1 Caching and Performance

    RemoteFileSystemModel will not fetch any files or directories until setRootPath()
    is called.  This will prevent any unnecessary querying on the file system
    until that point such as listing the drives on Windows.

    Unlike QDirModel, RemoteFileSystemModel uses a separate thread to populate
    itself so it will not cause the main thread to hang as the file system
    is being queried.  Calls to rowCount() will return 0 until the model
    populates a directory.

    RemoteFileSystemModel keeps a cache with file information. The cache is
    automatically kept up to date using the QFileSystemWatcher.

    \sa {Model Classes}
*/

/*!
    \fn bool RemoteFileSystemModel::rmdir(const QModelIndex &index)

    Removes the directory corresponding to the model item \a index in the
    file system model and \b{deletes the corresponding directory from the
    file system}, returning true if successful. If the directory cannot be
    removed, false is returned.

    \warning This function deletes directories from the file system; it does
    \b{not} move them to a location where they can be recovered.

    \sa remove()
*/

/*!
    \fn QIcon RemoteFileSystemModel::fileName(const QModelIndex &index) const

    Returns the file name for the item stored in the model under the given
    \a index.
*/

/*!
    \fn QIcon RemoteFileSystemModel::fileIcon(const QModelIndex &index) const

    Returns the icon for the item stored in the model under the given
    \a index.
*/

/*!
    \fn FileInfo RemoteFileSystemModel::fileInfo(const QModelIndex &index) const

    Returns the FileInfo for the item stored in the model under the given
    \a index.
*/
FileInfo RemoteFileSystemModel::fileInfo(const QModelIndex &index) const
{
    Q_D(const RemoteFileSystemModel);
    return d->node(index)->fileInfo();
}

/*!
    \fn void RemoteFileSystemModel::rootPathChanged(const QString &newPath);

    This signal is emitted whenever the root path has been changed to a \a newPath.
*/

/*!
    \fn void RemoteFileSystemModel::fileRenamed(const QString &path, const QString &oldName, const QString &newName)

    This signal is emitted whenever a file with the \a oldName is successfully
    renamed to \a newName.  The file is located in in the directory \a path.
*/

/*!
    \since 4.7
    \fn void RemoteFileSystemModel::directoryLoaded(const QString &path)

    This signal is emitted when the gatherer thread has finished to load the \a path.

*/


/*!
  Constructs a file system model with the given \a parent.
*/
RemoteFileSystemModel::RemoteFileSystemModel(AbstractFileInfoGatherer *fileinfogatherer, QObject *parent)
: AbstractFileSystemModel(parent), vd_ptr(new RemoteFileSystemModelPrivate(this, fileinfogatherer))
{
    Q_D(RemoteFileSystemModel);
    d->init();
}

#if 0
/*!
    \internal
*/
RemoteFileSystemModel::RemoteFileSystemModel(RemoteFileSystemModelPrivate & dd, QObject * parent)
    : QAbstractItemModel(dd, parent) {
    Q_D(RemoteFileSystemModel);
    d->init();
}
#endif

/*!
  Destroys this file system model.
*/
RemoteFileSystemModel::~RemoteFileSystemModel()
{}

/*!
    \reimp
*/
QModelIndex RemoteFileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const RemoteFileSystemModel);
    if (row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent))
        return QModelIndex();

    // get the parent node
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *parentNode =
        (indexValid(parent) ? d->node(parent)
                            : const_cast<RemoteFileSystemModelPrivate::RemoteFileSystemNode *>(&d->root));
    Q_ASSERT(parentNode);

    // now get the internal pointer for the index
    const int i = d->translateVisibleLocation(parentNode, row);
    if (i >= parentNode->visibleChildren.size())
        return QModelIndex();
    const QString &childName = parentNode->visibleChildren.at(i);
    const RemoteFileSystemModelPrivate::RemoteFileSystemNode *indexNode = parentNode->children.value(childName);
    Q_ASSERT(indexNode);

    return createIndex(row, column, const_cast<RemoteFileSystemModelPrivate::RemoteFileSystemNode *>(indexNode));
}

/*!
    \reimp
*/
QModelIndex RemoteFileSystemModel::sibling(int row, int column, const QModelIndex &idx) const
{
    if (row == idx.row() && column < RemoteFileSystemModelPrivate::NumColumns) {
        // cheap sibling operation: just adjust the column:
        return createIndex(row, column, idx.internalPointer());
    } else {
        // for anything else: call the default implementation
        // (this could probably be optimized, too):
        return QAbstractItemModel::sibling(row, column, idx);
    }
}

/*!
    \overload

    Returns the model item index for the given \a path and \a column.
*/
QModelIndex RemoteFileSystemModel::fsIndex(const QString &path, int column) const
{
    Q_D(const RemoteFileSystemModel);
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *node = d->node(path, false);
    return d->index(node, column);
}

/*!
    \internal

    Return the RemoteFileSystemNode that goes to index.
  */
RemoteFileSystemModelPrivate::RemoteFileSystemNode *RemoteFileSystemModelPrivate::node(const QModelIndex &index) const
{
    if (!index.isValid())
        return const_cast<RemoteFileSystemNode *>(&root);
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *indexNode =
        static_cast<RemoteFileSystemModelPrivate::RemoteFileSystemNode *>(index.internalPointer());
    Q_ASSERT(indexNode);
    return indexNode;
}

#if 0
static QString qt_GetLongPathName(const QString & strShortPath) {
    if (strShortPath.isEmpty()
        || strShortPath == QLatin1String(".") || strShortPath == QLatin1String(".."))
        return strShortPath;
    if (strShortPath.length() == 2 && strShortPath.endsWith(QLatin1Char(':')))
        return strShortPath.toUpper();
    const QString absPath = QDir(strShortPath).absolutePath();
    if (absPath.startsWith(QLatin1String("//"))
        || absPath.startsWith(QLatin1String("\\\\"))) // unc
        return QDir::fromNativeSeparators(absPath);
    if (absPath.startsWith(QLatin1Char('/')))
        return QString();
    const QString inputString = QLatin1String("\\\\?\\") + QDir::toNativeSeparators(absPath);
    QVarLengthArray<TCHAR, MAX_PATH> buffer(MAX_PATH);
    DWORD result = ::GetLongPathName((wchar_t*)inputString.utf16(),
        buffer.data(),
        buffer.size());
    if (result > DWORD(buffer.size())) {
        buffer.resize(result);
        result = ::GetLongPathName((wchar_t*)inputString.utf16(),
            buffer.data(),
            buffer.size());
    }
    if (result > 4) {
        QString longPath = QString::fromWCharArray(buffer.data() + 4); // ignoring prefix
        longPath[0] = longPath.at(0).toUpper(); // capital drive letters
        return QDir::fromNativeSeparators(longPath);
    } else {
        return QDir::fromNativeSeparators(strShortPath);
    }
}
#endif

/*!
    \internal

    Given a path return the matching RemoteFileSystemNode or &root if invalid
*/
RemoteFileSystemModelPrivate::RemoteFileSystemNode *RemoteFileSystemModelPrivate::node(const QString &path,
                                                                                       bool fetch) const
{
    Q_Q(const RemoteFileSystemModel);
    if (path.isEmpty() || path == myComputer() || path.startsWith(QLatin1Char(':')))
        return const_cast<RemoteFileSystemModelPrivate::RemoteFileSystemNode *>(&root);

        // Construct the nodes up to the new root path if they need to be built
#if 0
#ifdef Q_OS_WIN32
    QString longPath = qt_GetLongPathName(path);
#else
    QString longPath = path;
#endif
#endif
    QString absolutePath;
    QString longPath = path;
    if (longPath == q->rootPath())
        absolutePath = q->rootPath();
    else
        absolutePath = longPath;

    // ### TODO can we use bool QAbstractFileEngine::caseSensitive() const?
    QStringList pathElements = absolutePath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathElements.isEmpty() && QDir::fromNativeSeparators(longPath) != QLatin1String("/"))
        return const_cast<RemoteFileSystemModelPrivate::RemoteFileSystemNode *>(&root);
    QModelIndex index = QModelIndex(); // start with "My Computer"
    QString elementPath;
    QChar separator = QLatin1Char('/');
    QString trailingSeparator;
    if (q->isWindows()) {
        if (absolutePath.startsWith(QLatin1String("//"))) { // UNC path
            QString host = QLatin1String("\\\\") + pathElements.constFirst();
            if (absolutePath == QDir::fromNativeSeparators(host))
                absolutePath.append(QLatin1Char('/'));
            if (longPath.endsWith(QLatin1Char('/')) && !absolutePath.endsWith(QLatin1Char('/')))
                absolutePath.append(QLatin1Char('/'));
            if (absolutePath.endsWith(QLatin1Char('/')))
                trailingSeparator = QLatin1String("\\");
            int r = 0;
            RemoteFileSystemModelPrivate::RemoteFileSystemNode *rootNode =
                const_cast<RemoteFileSystemModelPrivate::RemoteFileSystemNode *>(&root);
            if (!root.children.contains(host.toLower())) {
                if (pathElements.count() == 1 && !absolutePath.endsWith(QLatin1Char('/')))
                    return rootNode;
                FileInfo info = fileInfoGatherer->getInfo(host);
                QModelIndex idx = q->fsIndex(host);
                if (!idx.isValid())
                    return rootNode;
                RemoteFileSystemModelPrivate *p = const_cast<RemoteFileSystemModelPrivate *>(this);
                p->addNode(rootNode, host, info);
                p->addVisibleFiles(rootNode, QStringList(host));
            }
            r = rootNode->visibleLocation(host);
            r = translateVisibleLocation(rootNode, r);
            index = q->index(r, 0, QModelIndex());
            pathElements.pop_front();
            separator = QLatin1Char('\\');
            elementPath = host;
            elementPath.append(separator);
        } else {
            if (pathElements.isEmpty()) {
                qDebug() << "invalid remote path received";
                return const_cast<RemoteFileSystemModelPrivate::RemoteFileSystemNode *>(&root);
            }
            if (!pathElements.at(0).contains(QLatin1Char(':'))) {
                QString rootPath = QDir(longPath).rootPath();
                pathElements.prepend(rootPath);
            }
            if (pathElements.at(0).endsWith(QLatin1Char('/')))
                pathElements[0].chop(1);
        }
    } else {
        // add the "/" item, since it is a valid path element on Unix
        if (absolutePath[0] == QLatin1Char('/'))
            pathElements.prepend(QLatin1String("/"));
    }

    RemoteFileSystemModelPrivate::RemoteFileSystemNode *parent = node(index);

    for (int i = 0; i < pathElements.count(); ++i) {
        QString element = pathElements.at(i);
        if (i != 0 && !elementPath.endsWith(separator))
            elementPath.append(separator);
        elementPath.append(element);
        if (i == pathElements.count() - 1)
            elementPath.append(trailingSeparator);
        if (q->isWindows()) {
            // On Windows, "filename    " and "filename" are equivalent and
            // "filename  .  " and "filename" are equivalent
            // "filename......." and "filename" are equivalent Task #133928
            // whereas "filename  .txt" is still "filename  .txt"
            // If after stripping the characters there is nothing left then we
            // just return the parent directory as it is assumed that the path
            // is referring to the parent
            while (element.endsWith(QLatin1Char('.')) || element.endsWith(QLatin1Char(' ')))
                element.chop(1);
            // Only filenames that can't possibly exist will be end up being empty
            if (element.isEmpty())
                return parent;
        }
        bool alreadyExisted = parent->children.contains(element);

        // we couldn't find the path element, we create a new node since we
        // _know_ that the path is valid
        if (alreadyExisted) {
            if ((parent->children.count() == 0) ||
                (parent->caseSensitive() && parent->children.value(element)->fileName != element) ||
                (!parent->caseSensitive() && parent->children.value(element)->fileName.toLower() != element.toLower()))
                alreadyExisted = false;
        }

        RemoteFileSystemModelPrivate::RemoteFileSystemNode *node = nullptr;
        if (!alreadyExisted) {
#if 0
            // FIXME
            // Someone might call ::index("file://cookie/monster/doesn't/like/veggies"),
            // a path that doesn't exists, I.E. don't blindly create directories.
            QFileInfo info(elementPath);
            if (!info.exists())
                return const_cast<RemoteFileSystemModelPrivate::RemoteFileSystemNode*>(&root);
#endif
            RemoteFileSystemModelPrivate *p = const_cast<RemoteFileSystemModelPrivate *>(this);
#ifndef QT_NO_FILESYSTEMWATCHER
            FileInfo fi = fileInfoGatherer->getInfo(elementPath);
#else
            FileInfo fi(elementPath);
#endif
            node = p->addNode(parent, element, fi);
        } else {
            node = parent->children.value(element);
        }

        Q_ASSERT(node);
        if (!node->isVisible()) {
            // It has been filtered out
            if (alreadyExisted && node->hasInformation() && !fetch)
                return const_cast<RemoteFileSystemModelPrivate::RemoteFileSystemNode *>(&root);

            RemoteFileSystemModelPrivate *p = const_cast<RemoteFileSystemModelPrivate *>(this);
            if (node->exists) {
                p->addVisibleFiles(parent, QStringList(element));
                if (!p->bypassFilters.contains(node))
                    p->bypassFilters[node] = 1;
            }
            QString dir = q->filePath(this->index(parent));
            if (!node->hasInformation() && fetch) {
                Fetching f = {std::move(dir), std::move(element), this->index(node)};
                p->toFetch.append(std::move(f));
                p->fetchingTimer.start(0, const_cast<RemoteFileSystemModel *>(q));
            }
        }
        parent = node;
    }

    return parent;
}

/*!
    \reimp
*/
void RemoteFileSystemModel::timerEvent(QTimerEvent *event)
{
    Q_D(RemoteFileSystemModel);
    if (event->timerId() == d->fetchingTimer.timerId()) {
        d->fetchingTimer.stop();
#ifndef QT_NO_FILESYSTEMWATCHER
        for (int i = 0; i < d->toFetch.count(); ++i) {
            const RemoteFileSystemModelPrivate::RemoteFileSystemNode *node = d->node(d->toFetch.at(i).nodeIndex);
            if (!node->hasInformation()) {
                d->fileInfoGatherer->fetchExtendedInformation(d->toFetch.at(i).dir, QStringList(d->toFetch.at(i).file));
            } else {
                // qDebug("yah!, you saved a little gerbil soul");
            }
        }
#endif
        d->toFetch.clear();
    }
}

/*!
    Returns \c true if the model item \a index represents a directory;
    otherwise returns \c false.
*/
bool RemoteFileSystemModel::isDir(const QModelIndex &index) const
{
    // This function is for public usage only because it could create a file info
    Q_D(const RemoteFileSystemModel);
    if (!index.isValid())
        return true;
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *n = d->node(index);
    if (n->hasInformation())
        return n->isDir();
    return fileInfo(index).isDir();
}

/*!
    Returns the size in bytes of \a index. If the file does not exist, 0 is returned.
  */
qint64 RemoteFileSystemModel::size(const QModelIndex &index) const
{
    Q_D(const RemoteFileSystemModel);
    if (!index.isValid())
        return 0;
    return d->node(index)->size();
}

/*!
    Returns the type of file \a index such as "Directory" or "JPEG file".
  */
QString RemoteFileSystemModel::type(const QModelIndex &index) const
{
    Q_D(const RemoteFileSystemModel);
    if (!index.isValid())
        return QString();
    return d->node(index)->type();
}

/*!
    Returns the date and time when \a index was last modified.
 */
QDateTime RemoteFileSystemModel::lastModified(const QModelIndex &index) const
{
    Q_D(const RemoteFileSystemModel);
    if (!index.isValid())
        return QDateTime();
    return d->node(index)->lastModified();
}

/*!
    \reimp
*/
QModelIndex RemoteFileSystemModel::parent(const QModelIndex &index) const
{
    Q_D(const RemoteFileSystemModel);
    if (!indexValid(index))
        return QModelIndex();

    RemoteFileSystemModelPrivate::RemoteFileSystemNode *indexNode = d->node(index);
    Q_ASSERT(indexNode != 0);
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *parentNode = indexNode->parent;
    if (parentNode == 0 || parentNode == &d->root)
        return QModelIndex();

    // get the parent's row
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *grandParentNode = parentNode->parent;
    Q_ASSERT(grandParentNode->children.contains(parentNode->fileName));
    int visualRow = d->translateVisibleLocation(
        grandParentNode,
        grandParentNode->visibleLocation(grandParentNode->children.value(parentNode->fileName)->fileName));
    if (visualRow == -1)
        return QModelIndex();
    return createIndex(visualRow, 0, parentNode);
}

/*
    \internal

    return the index for node
*/
QModelIndex RemoteFileSystemModelPrivate::index(const RemoteFileSystemModelPrivate::RemoteFileSystemNode *node,
                                                int column) const
{
    Q_Q(const RemoteFileSystemModel);
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *parentNode = (node ? node->parent : 0);
    if (node == &root || !parentNode)
        return QModelIndex();

    // get the parent's row
    Q_ASSERT(node);
    if (!node->isVisible())
        return QModelIndex();

    int visualRow = translateVisibleLocation(parentNode, parentNode->visibleLocation(node->fileName));
    return q->createIndex(visualRow, column, const_cast<RemoteFileSystemNode *>(node));
}

/*!
    \reimp
*/
bool RemoteFileSystemModel::hasChildren(const QModelIndex &parent) const
{
    Q_D(const RemoteFileSystemModel);
    if (parent.column() > 0)
        return false;

    if (!parent.isValid()) // drives
        return true;

    const RemoteFileSystemModelPrivate::RemoteFileSystemNode *indexNode = d->node(parent);
    Q_ASSERT(indexNode);
    return (indexNode->isDir());
}

/*!
    \reimp
 */
bool RemoteFileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    Q_D(const RemoteFileSystemModel);
    if (!d->setRootPath)
        return false;
    const RemoteFileSystemModelPrivate::RemoteFileSystemNode *indexNode = d->node(parent);
    return (!indexNode->populatedChildren);
}

/*!
    \reimp
 */
void RemoteFileSystemModel::fetchMore(const QModelIndex &parent)
{
    Q_D(RemoteFileSystemModel);
    if (!d->setRootPath)
        return;
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *indexNode = d->node(parent);
    if (indexNode->populatedChildren)
        return;
    indexNode->populatedChildren = true;
#ifndef QT_NO_FILESYSTEMWATCHER
    d->fileInfoGatherer->list(filePath(parent));
#endif
}

/*!
    \reimp
*/
int RemoteFileSystemModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const RemoteFileSystemModel);
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return d->root.visibleChildren.count();

    const RemoteFileSystemModelPrivate::RemoteFileSystemNode *parentNode = d->node(parent);
    return parentNode->visibleChildren.count();
}

/*!
    \reimp
*/
int RemoteFileSystemModel::columnCount(const QModelIndex &parent) const
{
    return (parent.column() > 0) ? 0 : RemoteFileSystemModelPrivate::NumColumns;
}

QString RemoteFileSystemModel::identifier() const
{
    Q_D(const RemoteFileSystemModel);
    return d->fileInfoGatherer->identifier();
}

/*!
    Returns the data stored under the given \a role for the item "My Computer".

    \sa Qt::ItemDataRole
 */
QVariant RemoteFileSystemModel::myComputer(int role) const
{
#ifndef QT_NO_FILESYSTEMWATCHER
    Q_D(const RemoteFileSystemModel);
#endif
    switch (role) {
    case Qt::DisplayRole: {
        auto h = d->fileInfoGatherer->hostname();
        if (!h.isEmpty()) {
            return h;
        }
        return RemoteFileSystemModelPrivate::myComputer();
    }
#ifndef QT_NO_FILESYSTEMWATCHER
    case Qt::DecorationRole:
        return d->fileInfoGatherer->iconProvider()->icon(RemoteFileIconProvider::Computer);
#endif
    }
    return QVariant();
}

QVariant RemoteFileSystemModel::homePath(int role) const
{
    Q_D(const RemoteFileSystemModel);
    switch (role) {
    case Qt::DisplayRole:
        if (!d->fileinfoGathererInitialized)
            qInfo() << "HOMEPATH not initialized" << d->fileInfoGatherer->homePath();
        return d->fileInfoGatherer->homePath();
        //return QString("~")+d->fileInfoGatherer->userName();
    case FilePathRole:
        if (!d->fileinfoGathererInitialized)
            qInfo() << "HOMEPATH not initialized" << d->fileInfoGatherer->homePath();
        return d->fileInfoGatherer->homePath();
    case Qt::DecorationRole:
        return qApp->style()->standardIcon(QStyle::SP_DirHomeIcon);
    }
    qInfo() << "RemoteFileSystemModel::homePath: role=" << role;
    return QVariant();
}


/*!
    \reimp
*/
QVariant RemoteFileSystemModel::data(const QModelIndex &index, int role) const
{
    Q_D(const RemoteFileSystemModel);
    if (!index.isValid() || index.model() != this)
        return QVariant();

    switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return d->displayName(index);
        case 1:
            return d->size(index);
        case 2:
            return d->type(index);
        case 3:
            return d->time(index);
        default:
            qWarning("data: invalid display value column %d", index.column());
            break;
        }
        break;
    case FilePathRole:
        return filePath(index);
    case FileNameRole:
        return d->name(index);
    case Qt::DecorationRole:
        if (index.column() == 0) {
            QIcon icon = d->icon(index);
#ifndef QT_NO_FILESYSTEMWATCHER
            if (icon.isNull()) {
                if (d->node(index)->isDir())
                    icon = d->fileInfoGatherer->iconProvider()->icon(RemoteFileIconProvider::Folder);
                else
                    icon = d->fileInfoGatherer->iconProvider()->icon(RemoteFileIconProvider::File);
            }
#endif // QT_NO_FILESYSTEMWATCHER
            return icon;
        }
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == 1)
            return QVariant(Qt::AlignTrailing | Qt::AlignVCenter);
        break;
    case FilePermissions:
        int p = permissions(index);
        return p;
    }

    return QVariant();
}

/*!
    \internal
*/
QString RemoteFileSystemModelPrivate::size(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    const RemoteFileSystemNode *n = node(index);
    if (n->isDir()) {
#ifdef Q_OS_MAC
        return QLatin1String("--");
#else
        return QLatin1String("");
#endif
        // Windows   - ""
        // OS X      - "--"
        // Konqueror - "4 KB"
        // Nautilus  - "9 items" (the number of children)
    }
    return size(n->size());
}

QString RemoteFileSystemModelPrivate::size(qint64 bytes)
{
    return QLocale::system().formattedDataSize(bytes);
}

/*!
    \internal
*/
QString RemoteFileSystemModelPrivate::time(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
#ifndef QT_NO_DATESTRING
    return node(index)->lastModified().toString();
#else
    Q_UNUSED(index);
    return QString();
#endif
}

void RemoteFileSystemModelPrivate::_q_modelInitialized()
{
    Q_Q(RemoteFileSystemModel);
    fileinfoGathererInitialized = true;
    emit q->initialized();
}

/*
    \internal
*/
QString RemoteFileSystemModelPrivate::type(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    return node(index)->type();
}

/*!
    \internal
*/
QString RemoteFileSystemModelPrivate::name(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();
    RemoteFileSystemNode *dirNode = node(index);
    if (
#ifndef QT_NO_FILESYSTEMWATCHER
        fileInfoGatherer->resolveSymlinks() &&
#endif
        !resolvedSymLinks.isEmpty() && dirNode->isSymLink(/* ignoreNtfsSymLinks = */ true)) {
        QString fullPath = QDir::fromNativeSeparators(filePath(index));
        return resolvedSymLinks.value(fullPath, dirNode->fileName);
    }
    return dirNode->fileName;
}

/*!
    \internal
*/
QString RemoteFileSystemModelPrivate::displayName(const QModelIndex &index) const
{
    Q_Q(const RemoteFileSystemModel);
    if (q->isWindows()) {
        RemoteFileSystemNode *dirNode = node(index);
        if (!dirNode->volumeName.isNull())
            return dirNode->volumeName + QLatin1String(" (") + name(index) + QLatin1Char(')');
    }
    return name(index);
}

/*!
    \internal
*/
QIcon RemoteFileSystemModelPrivate::icon(const QModelIndex &index) const
{
    if (!index.isValid())
        return QIcon();
    return node(index)->icon();
}


/*!
    \reimp
*/
QVariant RemoteFileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DecorationRole:
        if (section == 0) {
            // ### TODO oh man this is ugly and doesn't even work all the way!
            // it is still 2 pixels off
            QImage pixmap(16, 1, QImage::Format_Mono);
            pixmap.fill(0);
            pixmap.setAlphaChannel(pixmap.createAlphaMask());
            return pixmap;
        }
        break;
    case Qt::TextAlignmentRole:
        return Qt::AlignLeft;
    }

    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractItemModel::headerData(section, orientation, role);

    QString returnValue;
    switch (section) {
    case 0:
        returnValue = tr("Name");
        break;
    case 1:
        returnValue = tr("Size");
        break;
    case 2:
        returnValue =
#ifdef Q_OS_MAC
            tr("Kind", "Match OS X Finder");
#else
            tr("Type", "All other platforms");
#endif
        break;
        // Windows   - Type
        // OS X      - Kind
        // Konqueror - File Type
        // Nautilus  - Type
    case 3:
        returnValue = tr("Date Modified");
        break;
    default:
        return QVariant();
    }
    return returnValue;
}

/*!
    \reimp
*/
Qt::ItemFlags RemoteFileSystemModel::flags(const QModelIndex &index) const
{
    Q_D(const RemoteFileSystemModel);
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (!index.isValid())
        return flags;

    RemoteFileSystemModelPrivate::RemoteFileSystemNode *indexNode = d->node(index);
    if (d->nameFilterDisables && !d->passNameFilters(indexNode)) {
        flags &= ~Qt::ItemIsEnabled;
        // ### TODO you shouldn't be able to set this as the current item, task 119433
        return flags;
    }
    const bool hideFiles = !(d->filters & QDir::Files);
    if (d->nameFilterDisables && hideFiles && !indexNode->isDir()) {
        flags &= ~Qt::ItemIsEnabled;
        return flags;
    }

    flags |= Qt::ItemIsDragEnabled;
    if (d->readOnly)
        return flags;
    if ((index.column() == 0) && indexNode->permissions() & QFile::WriteUser) {
        //flags |= Qt::ItemIsEditable;
        if (indexNode->isDir()) {
            //flags |= Qt::ItemIsDropEnabled;
        } else {
            flags |= Qt::ItemNeverHasChildren;
        }
    }
    return flags;
}

/*!
    \internal
*/
void RemoteFileSystemModelPrivate::_q_performDelayedSort()
{
    Q_Q(RemoteFileSystemModel);
    q->sort(sortColumn, sortOrder);
}


/*
    \internal
    Helper functor used by sort()
*/
class QFileSystemModelSorter {
public:
    inline QFileSystemModelSorter(int column): sortColumn(column)
    {
        naturalCompare.setNumericMode(true);
        naturalCompare.setCaseSensitivity(Qt::CaseInsensitive);
    }

    bool compareNodes(const RemoteFileSystemModelPrivate::RemoteFileSystemNode *l,
                      const RemoteFileSystemModelPrivate::RemoteFileSystemNode *r) const
    {
        switch (sortColumn) {
        case 0: {
#ifndef Q_OS_MAC
            // place directories before files
            bool left = l->isDir();
            bool right = r->isDir();
            if (left ^ right)
                return left;
#endif
            return naturalCompare.compare(l->fileName, r->fileName) < 0;
        }
        case 1: {
            // Directories go first
            bool left = l->isDir();
            bool right = r->isDir();
            if (left ^ right)
                return left;

            qint64 sizeDifference = l->size() - r->size();
            if (sizeDifference == 0)
                return naturalCompare.compare(l->fileName, r->fileName) < 0;

            return sizeDifference < 0;
        }
        case 2: {
            int compare = naturalCompare.compare(l->type(), r->type());
            if (compare == 0)
                return naturalCompare.compare(l->fileName, r->fileName) < 0;

            return compare < 0;
        }
        case 3: {
            if (l->lastModified() == r->lastModified())
                return naturalCompare.compare(l->fileName, r->fileName) < 0;

            return l->lastModified() < r->lastModified();
        }
        }
        Q_ASSERT(false);
        return false;
    }

    bool operator()(const RemoteFileSystemModelPrivate::RemoteFileSystemNode *l,
                    const RemoteFileSystemModelPrivate::RemoteFileSystemNode *r) const
    {
        return compareNodes(l, r);
    }


private:
    QCollator naturalCompare;
    int sortColumn;
};

/*
    \internal

    Sort all of the children of parent
*/
void RemoteFileSystemModelPrivate::sortChildren(int column, const QModelIndex &parent)
{
    Q_Q(RemoteFileSystemModel);
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *indexNode = node(parent);
    if (indexNode->children.count() == 0)
        return;

    QVector<RemoteFileSystemModelPrivate::RemoteFileSystemNode *> values;

    for (auto iterator = indexNode->children.constBegin(), cend = indexNode->children.constEnd(); iterator != cend;
         ++iterator) {
        if (filtersAcceptsNode(iterator.value())) {
            values.append(iterator.value());
        } else {
            iterator.value()->m_isVisible = false;
        }
    }
    QFileSystemModelSorter ms(column);
    std::sort(values.begin(), values.end(), ms);
    // First update the new visible list
    indexNode->visibleChildren.clear();
    //No more dirty item we reset our internal dirty index
    indexNode->dirtyChildrenIndex = -1;
    const int numValues = values.count();
    indexNode->visibleChildren.reserve(numValues);
    for (int i = 0; i < numValues; ++i) {
        indexNode->visibleChildren.append(values.at(i)->fileName);
        values.at(i)->m_isVisible = true;
    }

    if (!disableRecursiveSort) {
        for (int i = 0; i < q->rowCount(parent); ++i) {
            const QModelIndex childIndex = q->index(i, 0, parent);
            RemoteFileSystemModelPrivate::RemoteFileSystemNode *indexNode = node(childIndex);
            //Only do a recursive sort on visible nodes
            if (indexNode->isVisible())
                sortChildren(column, childIndex);
        }
    }
}

/*!
    \reimp
*/
void RemoteFileSystemModel::sort(int column, Qt::SortOrder order)
{
    Q_D(RemoteFileSystemModel);
    if (d->sortOrder == order && d->sortColumn == column && !d->forceSort)
        return;

    emit layoutAboutToBeChanged();
    QModelIndexList oldList = persistentIndexList();
    QVector<QPair<RemoteFileSystemModelPrivate::RemoteFileSystemNode *, int>> oldNodes;
    const int nodeCount = oldList.count();
    oldNodes.reserve(nodeCount);
    for (int i = 0; i < nodeCount; ++i) {
        const QModelIndex &oldNode = oldList.at(i);
        QPair<RemoteFileSystemModelPrivate::RemoteFileSystemNode *, int> pair(d->node(oldNode), oldNode.column());
        oldNodes.append(pair);
    }

    if (!(d->sortColumn == column && d->sortOrder != order && !d->forceSort)) {
        //we sort only from where we are, don't need to sort all the model
        d->sortChildren(column, fsIndex(rootPath()));
        d->sortColumn = column;
        d->forceSort = false;
    }
    d->sortOrder = order;

    QModelIndexList newList;
    const int numOldNodes = oldNodes.size();
    newList.reserve(numOldNodes);
    for (int i = 0; i < numOldNodes; ++i) {
        const QPair<RemoteFileSystemModelPrivate::RemoteFileSystemNode *, int> &oldNode = oldNodes.at(i);
        newList.append(d->index(oldNode.first, oldNode.second));
    }
    changePersistentIndexList(oldList, newList);
    emit layoutChanged();
}

/*!
    Returns a list of MIME types that can be used to describe a list of items
    in the model.
*/
QStringList RemoteFileSystemModel::mimeTypes() const
{
    return QStringList(QLatin1String("text/uri-list"));
}

/*!
    Returns an object that contains a serialized description of the specified
    \a indexes. The format used to describe the items corresponding to the
    indexes is obtained from the mimeTypes() function.

    If the list of indexes is empty, 0 is returned rather than a serialized
    empty list.
*/
QMimeData *RemoteFileSystemModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> urls;
    QList<QModelIndex>::const_iterator it = indexes.begin();
    for (; it != indexes.end(); ++it)
        if ((*it).column() == 0)
            urls << QUrl::fromLocalFile(filePath(*it));
    QMimeData *data = new QMimeData();
    data->setUrls(urls);
    return data;
}


/*!
    \reimp
*/
Qt::DropActions RemoteFileSystemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}

bool RemoteFileSystemModel::isRootDir(const QString &path) const
{
    Q_D(const RemoteFileSystemModel);
    return d->fileInfoGatherer->isRootDir(path);
}

QString RemoteFileSystemModel::workingDirectory() const
{
    Q_D(const RemoteFileSystemModel);
    return d->fileInfoGatherer->workingDirectory();
}

bool RemoteFileSystemModel::isWindows() const
{
    Q_D(const RemoteFileSystemModel);
    return d->fileInfoGatherer->isWindows();
}

QString RemoteFileSystemModel::userName() const
{
    Q_D(const RemoteFileSystemModel);
    return d->fileInfoGatherer->userName();
}

/*!
    Returns the path of the item stored in the model under the
    \a index given.
*/
QString RemoteFileSystemModel::filePath(const QModelIndex &index) const
{
    Q_D(const RemoteFileSystemModel);
    QString fullPath = d->filePath(index);
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *dirNode = d->node(index);
    if (dirNode->isSymLink()
#ifndef QT_NO_FILESYSTEMWATCHER
        && d->fileInfoGatherer->resolveSymlinks()
#endif
        && d->resolvedSymLinks.contains(fullPath) && dirNode->isDir()) {
        return dirNode->linkTarget();
    }
    return fullPath;
}

QString RemoteFileSystemModelPrivate::filePath(const QModelIndex &index) const
{
    Q_Q(const RemoteFileSystemModel);
    if (!index.isValid())
        return QString();
    Q_ASSERT(index.model() == q);

    QStringList path;
    QModelIndex idx = index;
    while (idx.isValid()) {
        RemoteFileSystemModelPrivate::RemoteFileSystemNode *dirNode = node(idx);
        if (dirNode)
            path.prepend(dirNode->fileName);
        idx = idx.parent();
    }
    QString fullPath = QDir::fromNativeSeparators(path.join(QDir::separator()));
    if (q->isWindows()) {
        if (fullPath.length() == 2 && fullPath.endsWith(QLatin1Char(':')))
            fullPath.append(QLatin1Char('/'));
    } else {
        if (!fullPath.startsWith(QLatin1Char('/')))
            fullPath = "/" + fullPath;
        if ((fullPath.length() > 2) && fullPath[0] == QLatin1Char('/') && fullPath[1] == QLatin1Char('/'))
            fullPath = fullPath.mid(1);
    }
    return QDir::cleanPath(fullPath);
}

/*!
    Create a directory with the \a name in the \a parent model index.
*/
QModelIndex RemoteFileSystemModel::mkdir(const QModelIndex &parent, const QString &name)
{
    Q_D(RemoteFileSystemModel);
    if (!parent.isValid())
        return parent;

    QString dir(filePath(parent));
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *parentNode = d->node(parent);
    d->addNode(parentNode, name, FileInfo(dir));
    Q_ASSERT(parentNode->children.contains(name));
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *node = parentNode->children[name];
#ifndef QT_NO_FILESYSTEMWATCHER
    QString newDir = QDir::cleanPath(dir + "/" + name);
    d->fileInfoGatherer->mkdir(newDir);
    node->populate(d->fileInfoGatherer->getInfo(newDir));
#endif
    d->addVisibleFiles(parentNode, QStringList(name));
    return d->index(node);
}

/*!
    Returns the complete OR-ed together combination of QFile::Permission for the \a index.
 */
QFile::Permissions RemoteFileSystemModel::permissions(const QModelIndex &index) const
{
    Q_D(const RemoteFileSystemModel);
    return d->node(index)->permissions();
}

/*!
    Sets the directory that is being watched by the model to \a newPath by
    installing a \l{QFileSystemWatcher}{file system watcher} on it. Any
    changes to files and directories within this directory will be
    reflected in the model.

    If the path is changed, the rootPathChanged() signal will be emitted.

    \note This function does not change the structure of the model or
    modify the data available to views. In other words, the "root" of
    the model is \e not changed to include only files and directories
    within the directory specified by \a newPath in the file system.
  */
QModelIndex RemoteFileSystemModel::setRootPath(const QString &newPath)
{
    Q_D(RemoteFileSystemModel);
    QString longNewPath = newPath;
    if (isWindows()) {
        longNewPath = QDir::fromNativeSeparators(newPath);
    }
    //QDir newPathDir(longNewPath);
    //we remove .. and . from the given path if exist
    if (!newPath.isEmpty()) {
        longNewPath = QDir::cleanPath(longNewPath);
        //newPathDir.setPath(longNewPath);
    }

    d->setRootPath = true;

    //user don't ask for the root path ("") but the conversion failed
    if (!newPath.isEmpty() && longNewPath.isEmpty()) {
        qDebug() << "setRootPath: newPath=" << newPath << ", longNewPath=" << longNewPath;
        return d->index(rootPath());
    }

    if (rootPath() == longNewPath) {
        qDebug() << "setRootPath: longNewPath=" << longNewPath << ", equals rootdir";
        return d->index(rootPath());
    }

    bool showDrives = (longNewPath.isEmpty() || longNewPath == d->myComputer());
#if 0
    if (!showDrives && !newPathDir.exists()) {
        qDebug() << "setRootPath: showDrives, newPathDir=" << newPathDir;
        return d->index(rootPath());
    }
#endif

    //We remove the watcher on the previous path
    if (!rootPath().isEmpty() && rootPath() != QLatin1String(".")) {
        //This remove the watcher for the old rootPath
#ifndef QT_NO_FILESYSTEMWATCHER
        d->fileInfoGatherer->removePath(rootPath());
#endif
        //This line "marks" the node as dirty, so the next fetchMore
        //call on the path will ask the gatherer to install a watcher again
        //But it doesn't re-fetch everything
        d->node(rootPath())->populatedChildren = false;
    }

    // We have a new valid root path
    //d->rootDir = newPathDir;
    d->rootPath = longNewPath;
    QModelIndex newRootIndex;
    if (showDrives) {
        // otherwise dir will become '.'
        d->rootPath = QLatin1String("");
    } else {
        newRootIndex = d->index(d->rootPath);
    }
    fetchMore(newRootIndex);
    emit rootPathChanged(longNewPath);
    d->forceSort = true;
    d->delayedSort();
    return newRootIndex;
}

/*!
    The currently set root path

    \sa rootDirectory()
*/
QString RemoteFileSystemModel::rootPath() const
{
    Q_D(const RemoteFileSystemModel);
    return d->rootPath;
}

#if 0
/*!
    The currently set directory

    \sa rootPath()
*/
QDir RemoteFileSystemModel::rootDirectory() const {
    Q_D(const RemoteFileSystemModel);
    QDir dir(d->rootDir);
    dir.setNameFilters(nameFilters());
    dir.setFilter(filter());
    return dir;
}
#endif

/*!
    Sets the \a provider of file icons for the directory model.
*/
void RemoteFileSystemModel::setIconProvider(RemoteFileIconProvider *provider)
{
    Q_D(RemoteFileSystemModel);
#ifndef QT_NO_FILESYSTEMWATCHER
    d->fileInfoGatherer->setIconProvider(provider);
#endif
    d->root.updateIcon(provider, QString());
}

/*!
    Returns the file icon provider for this directory model.
*/
RemoteFileIconProvider *RemoteFileSystemModel::iconProvider() const
{
#ifndef QT_NO_FILESYSTEMWATCHER
    Q_D(const RemoteFileSystemModel);
    return d->fileInfoGatherer->iconProvider();
#else
    return 0;
#endif
}

void RemoteFileSystemModel::setDisableRecursiveSort(bool enable)
{
    Q_D(RemoteFileSystemModel);
    d->disableRecursiveSort = enable;
}

bool RemoteFileSystemModel::disableRecursiveSort() const
{
    Q_D(const RemoteFileSystemModel);
    return d->disableRecursiveSort;
}

/*!
    Sets the directory model's filter to that specified by \a filters.

    Note that the filter you set should always include the QDir::AllDirs enum value,
    otherwise RemoteFileSystemModel won't be able to read the directory structure.

    \sa QDir::Filters
*/
void RemoteFileSystemModel::setFilter(QDir::Filters filters)
{
    Q_D(RemoteFileSystemModel);
    if (d->filters == filters)
        return;
    d->filters = filters;
    // CaseSensitivity might have changed
    setNameFilters(nameFilters());
    d->forceSort = true;
    d->delayedSort();
}

/*!
    Returns the filter specified for the directory model.

    If a filter has not been set, the default filter is QDir::AllEntries |
    QDir::NoDotAndDotDot | QDir::AllDirs.

    \sa QDir::Filters
*/
QDir::Filters RemoteFileSystemModel::filter() const
{
    Q_D(const RemoteFileSystemModel);
    return d->filters;
}

/*!
    \property RemoteFileSystemModel::resolveSymlinks
    \brief Whether the directory model should resolve symbolic links

    This is only relevant on Windows.

    By default, this property is \c true.
*/
void RemoteFileSystemModel::setResolveSymlinks(bool enable)
{
#ifndef QT_NO_FILESYSTEMWATCHER
    Q_D(RemoteFileSystemModel);
    d->fileInfoGatherer->setResolveSymlinks(enable);
#else
    Q_UNUSED(enable)
#endif
}

bool RemoteFileSystemModel::resolveSymlinks() const
{
#ifndef QT_NO_FILESYSTEMWATCHER
    Q_D(const RemoteFileSystemModel);
    return d->fileInfoGatherer->resolveSymlinks();
#else
    return false;
#endif
}

/*!
    \property RemoteFileSystemModel::readOnly
    \brief Whether the directory model allows writing to the file system

    If this property is set to false, the directory model will allow renaming, copying
    and deleting of files and directories.

    This property is \c true by default
*/
void RemoteFileSystemModel::setReadOnly(bool enable)
{
    Q_D(RemoteFileSystemModel);
    d->readOnly = enable;
}

bool RemoteFileSystemModel::isReadOnly() const
{
    Q_D(const RemoteFileSystemModel);
    return d->readOnly;
}

/*!
    \property RemoteFileSystemModel::nameFilterDisables
    \brief Whether files that don't pass the name filter are hidden or disabled

    This property is \c true by default
*/
void RemoteFileSystemModel::setNameFilterDisables(bool enable)
{
    Q_D(RemoteFileSystemModel);
    if (d->nameFilterDisables == enable)
        return;
    d->nameFilterDisables = enable;
    d->forceSort = true;
    d->delayedSort();
}

bool RemoteFileSystemModel::nameFilterDisables() const
{
    Q_D(const RemoteFileSystemModel);
    return d->nameFilterDisables;
}

/*!
    Sets the name \a filters to apply against the existing files.
*/
void RemoteFileSystemModel::setNameFilters(const QStringList &filters)
{
    // Prep the regexp's ahead of time
#ifndef QT_NO_REGEXP
    Q_D(RemoteFileSystemModel);

    if (!d->bypassFilters.isEmpty()) {
        // update the bypass filter to only bypass the stuff that must be kept around
        d->bypassFilters.clear();
        // We guarantee that rootPath will stick around
        QPersistentModelIndex root(fsIndex(rootPath()));
        const QModelIndexList persistentList = persistentIndexList();
        for (const auto &persistentIndex: persistentList) {
            RemoteFileSystemModelPrivate::RemoteFileSystemNode *node = d->node(persistentIndex);
            while (node) {
                if (d->bypassFilters.contains(node))
                    break;
                if (node->isDir())
                    d->bypassFilters[node] = true;
                node = node->parent;
            }
        }
    }

    d->nameFilters.clear();
    const QRegularExpression::PatternOption caseSensitive = (filter() & QDir::CaseSensitive)
                                                                ? QRegularExpression::NoPatternOption
                                                                : QRegularExpression::CaseInsensitiveOption;
    for (const auto &filter: filters)
        d->nameFilters << QRegularExpression(QRegularExpression::wildcardToRegularExpression(filter), caseSensitive);
    d->forceSort = true;
    d->delayedSort();
#endif
}

/*!
    Returns a list of filters applied to the names in the model.
*/
QStringList RemoteFileSystemModel::nameFilters() const
{
    Q_D(const RemoteFileSystemModel);
    QStringList filters;
#ifndef QT_NO_REGEXP
    const int numNameFilters = d->nameFilters.size();
    filters.reserve(numNameFilters);
    for (int i = 0; i < numNameFilters; ++i) {
        filters << d->nameFilters.at(i).pattern();
    }
#endif
    return filters;
}

/*!
    \reimp
*/
bool RemoteFileSystemModel::event(QEvent *event)
{
#ifndef QT_NO_FILESYSTEMWATCHER
    Q_D(RemoteFileSystemModel);
    if (event->type() == QEvent::LanguageChange) {
        d->root.retranslateStrings(d->fileInfoGatherer->iconProvider(), QString());
        return true;
    }
#endif
    return QAbstractItemModel::event(event);
}

bool RemoteFileSystemModel::indexValid(const QModelIndex &index) const
{
    return index.row() >= 0 && index.column() >= 0 && index.model() == this;
}


/*!
     \internal

    Performed quick listing and see if any files have been added or removed,
    then fetch more information on visible files.
 */
void RemoteFileSystemModelPrivate::_q_directoryChanged(const QString &directory, const QStringList &files)
{
    //Q_Q(RemoteFileSystemModel);

    RemoteFileSystemModelPrivate::RemoteFileSystemNode *parentNode = node(directory, true);
    if (!parentNode->children.isEmpty()) {
        QStringList toRemove;
        QStringList newFiles = files;
        std::sort(newFiles.begin(), newFiles.end());
        for (auto i = parentNode->children.constBegin(), cend = parentNode->children.constEnd(); i != cend; ++i) {
            QStringList::iterator iterator = std::lower_bound(newFiles.begin(), newFiles.end(), i.value()->fileName);
            if ((iterator == newFiles.end()) || (i.value()->fileName < *iterator))
                toRemove.append(i.value()->fileName);
        }
        for (int i = 0; i < toRemove.count(); ++i)
            removeNode(parentNode, toRemove[i]);
    }

#if 0
    for (auto& f : files) {
        FileInfo fi = fileInfoGatherer->getInfo(QDir::cleanPath(directory + "/" + f));
        fi.m_exists = true;
        auto node = addNode(parentNode, f, fi);
#if 0
        if (!node->hasInformation()) {
            Fetching fetch = { directory, f, node };
            toFetch.append(std::move(fetch));
    }
#endif
}
    //fetchingTimer.start(0, q);
#endif
}

/*!
    \internal

    Adds a new file to the children of parentNode

    *WARNING* this will change the count of children
*/
RemoteFileSystemModelPrivate::RemoteFileSystemNode *
RemoteFileSystemModelPrivate::addNode(RemoteFileSystemNode *parentNode, const QString &fileName, const FileInfo &info)
{
    Q_Q(RemoteFileSystemModel);
    // In the common case, itemLocation == count() so check there first
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *node =
        new RemoteFileSystemModelPrivate::RemoteFileSystemNode(q, fileName, parentNode);
#ifndef QT_NO_FILESYSTEMWATCHER
    node->populate(info);
#else
    Q_UNUSED(info)
#endif
    Q_ASSERT(!parentNode->children.contains(fileName));
    parentNode->children.insert({fileName, info.isCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive}, node);
    return node;
}

/*!
    \internal

    File at parentNode->children(itemLocation) has been removed, remove from the lists
    and emit signals if necessary

    *WARNING* this will change the count of children and could change visibleChildren
 */
void RemoteFileSystemModelPrivate::removeNode(RemoteFileSystemModelPrivate::RemoteFileSystemNode *parentNode,
                                              const QString &name)
{
    Q_Q(RemoteFileSystemModel);
    QModelIndex parent = index(parentNode);
    bool indexHidden = isHiddenByFilter(parentNode, parent);

    int vLocation = parentNode->visibleLocation(name);
    if (vLocation >= 0 && !indexHidden)
        q->beginRemoveRows(parent, translateVisibleLocation(parentNode, vLocation),
                           translateVisibleLocation(parentNode, vLocation));
    RemoteFileSystemNode *node = parentNode->children.take(name);
    delete node;
    // cleanup sort files after removing rather then re-sorting which is O(n)
    if (vLocation >= 0)
        parentNode->visibleChildren.removeAt(vLocation);
    if (vLocation >= 0 && !indexHidden)
        q->endRemoveRows();
}

/*!
    \internal

    File at parentNode->children(itemLocation) was not visible before, but now should be
    and emit signals if necessary.

    *WARNING* this will change the visible count
 */
void RemoteFileSystemModelPrivate::addVisibleFiles(RemoteFileSystemNode *parentNode, const QStringList &newFiles)
{
    Q_Q(RemoteFileSystemModel);
    QModelIndex parent = index(parentNode);
    bool indexHidden = isHiddenByFilter(parentNode, parent);
    if (!indexHidden) {
        q->beginInsertRows(parent, parentNode->visibleChildren.count(),
                           parentNode->visibleChildren.count() + newFiles.count() - 1);
    }

    if (parentNode->dirtyChildrenIndex == -1)
        parentNode->dirtyChildrenIndex = parentNode->visibleChildren.count();

    for (const auto &newFile: newFiles) {
        parentNode->visibleChildren.append(newFile);
        parentNode->children.value(newFile)->m_isVisible = true;
    }
    if (!indexHidden)
        q->endInsertRows();
}

/*!
    \internal

    File was visible before, but now should NOT be

    *WARNING* this will change the visible count
 */
void RemoteFileSystemModelPrivate::removeVisibleFile(RemoteFileSystemNode *parentNode, int vLocation)
{
    Q_Q(RemoteFileSystemModel);
    if (vLocation == -1)
        return;
    QModelIndex parent = index(parentNode);
    bool indexHidden = isHiddenByFilter(parentNode, parent);
    if (!indexHidden)
        q->beginRemoveRows(parent, translateVisibleLocation(parentNode, vLocation),
                           translateVisibleLocation(parentNode, vLocation));
    parentNode->children.value(parentNode->visibleChildren.at(vLocation))->m_isVisible = false;
    parentNode->visibleChildren.removeAt(vLocation);
    if (!indexHidden)
        q->endRemoveRows();
}

/*!
    \internal

    The thread has received new information about files,
    update and emit dataChanged if it has actually changed.
 */
void RemoteFileSystemModelPrivate::_q_fileSystemChanged(const QString &path,
                                                        const QVector<QPair<QString, FileInfo>> &updates)
{
#ifndef QT_NO_FILESYSTEMWATCHER
    Q_Q(RemoteFileSystemModel);
    QVector<QString> rowsToUpdate;
    QStringList newFiles;
    RemoteFileSystemModelPrivate::RemoteFileSystemNode *parentNode = node(path, false);
    QModelIndex parentIndex = index(parentNode);
    for (const auto &update: updates) {
        QString fileName = update.first;
        Q_ASSERT(!fileName.isEmpty());
        FileInfo info = update.second;
        bool previouslyHere = parentNode->children.contains(fileName);
        if (!previouslyHere) {
            addNode(parentNode, fileName, info);
        }
        RemoteFileSystemModelPrivate::RemoteFileSystemNode *node = parentNode->children.value(fileName);
        bool isCaseSensitive = parentNode->caseSensitive();
        if (isCaseSensitive) {
            if (node->fileName != fileName)
                continue;
        } else {
            if (QString::compare(node->fileName, fileName, Qt::CaseInsensitive) != 0)
                continue;
        }
        if (isCaseSensitive) {
            Q_ASSERT(node->fileName == fileName);
        } else {
            node->fileName = fileName;
        }

        if (!previouslyHere || *node != info) {
            node->populate(info);
            bypassFilters.remove(node);
            // brand new information.
            if (filtersAcceptsNode(node)) {
                if (!node->isVisible()) {
                    newFiles.append(fileName);
                } else {
                    rowsToUpdate.append(fileName);
                }
            } else {
                if (node->isVisible()) {
                    int visibleLocation = parentNode->visibleLocation(fileName);
                    removeVisibleFile(parentNode, visibleLocation);
                } else {
                    // The file is not visible, don't do anything
                }
            }
        }
    }

    // bundle up all of the changed signals into as few as possible.
    std::sort(rowsToUpdate.begin(), rowsToUpdate.end());
    QString min;
    QString max;
    for (int i = 0; i < rowsToUpdate.count(); ++i) {
        QString value = rowsToUpdate.at(i);
        //##TODO is there a way to bundle signals with QString as the content of the list?

        /*if (min.isEmpty()) {
            min = value;
            if (i != rowsToUpdate.count() - 1)
                continue;
        }
        if (i != rowsToUpdate.count() - 1) {
            if ((value == min + 1 && max.isEmpty()) || value == max + 1) {
                max = value;
                continue;
            }
        }*/
        max = value;
        min = value;
        int visibleMin = parentNode->visibleLocation(min);
        int visibleMax = parentNode->visibleLocation(max);
        if (visibleMin >= 0 && visibleMin < parentNode->visibleChildren.count() &&
            parentNode->visibleChildren.at(visibleMin) == min && visibleMax >= 0) {
            QModelIndex bottom = q->index(translateVisibleLocation(parentNode, visibleMin), 0, parentIndex);
            QModelIndex top = q->index(translateVisibleLocation(parentNode, visibleMax), 3, parentIndex);
            emit q->dataChanged(bottom, top);
        }

        /*min = QString();
        max = QString();*/
    }

    if (newFiles.count() > 0) {
        addVisibleFiles(parentNode, newFiles);
    }

    if (newFiles.count() > 0 || (sortColumn != 0 && rowsToUpdate.count() > 0)) {
        forceSort = true;
        delayedSort();
    }
#else
    Q_UNUSED(path)
    Q_UNUSED(updates)
#endif // !QT_NO_FILESYSTEMWATCHER
}

/*!
    \internal
*/
void RemoteFileSystemModelPrivate::_q_resolvedName(const QString &fileName, const QString &resolvedName)
{
    resolvedSymLinks[fileName] = resolvedName;
}
/*
#if QT_CONFIG(filesystemwatcher) && defined(Q_OS_WIN)
// Remove file system watchers at/below the index and return a list of previously
// watched files. This should be called prior to operations like rename/remove
// which might fail due to watchers on platforms like Windows. The watchers
// should be restored on failure.
QStringList RemoteFileSystemModelPrivate::unwatchPathsAt(const QModelIndex &index)
{
    const RemoteFileSystemModelPrivate::RemoteFileSystemNode *indexNode = node(index);
    if (indexNode == nullptr)
        return QStringList();
    const Qt::CaseSensitivity caseSensitivity = indexNode->caseSensitive()
        ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const QString path = indexNode->fileInfo().absoluteFilePath();

    QStringList result;
    const auto filter = [path, caseSensitivity] (const QString &watchedPath)
    {
        const int pathSize = path.size();
        if (pathSize == watchedPath.size()) {
            return path.compare(watchedPath, caseSensitivity) == 0;
        } else if (watchedPath.size() > pathSize) {
            return watchedPath.at(pathSize) == QLatin1Char('/')
                && watchedPath.startsWith(path, caseSensitivity);
        }
        return false;
    };

    const QStringList &watchedFiles = fileInfoGatherer->watchedFiles();
    std::copy_if(watchedFiles.cbegin(), watchedFiles.cend(),
                 std::back_inserter(result), filter);

    const QStringList &watchedDirectories = fileInfoGatherer->watchedDirectories();
    std::copy_if(watchedDirectories.cbegin(), watchedDirectories.cend(),
                 std::back_inserter(result), filter);

    fileInfoGatherer->unwatchPaths(result);
    return result;
}
#endif // filesystemwatcher && Q_OS_WIN
*/
/*!
    \internal
*/
void RemoteFileSystemModelPrivate::init()
{
    Q_Q(RemoteFileSystemModel);
    qRegisterMetaType<QVector<QPair<QString, FileInfo>>>();
    q->connect(fileInfoGatherer, SIGNAL(initialized()), q, SLOT(_q_modelInitialized()));
#ifndef QT_NO_FILESYSTEMWATCHER
    q->connect(fileInfoGatherer, SIGNAL(newListOfFiles(QString, QStringList)), q,
               SLOT(_q_directoryChanged(QString, QStringList)));
    q->connect(fileInfoGatherer, SIGNAL(updates(QString, QVector<QPair<QString, FileInfo>>)), q,
               SLOT(_q_fileSystemChanged(QString, QVector<QPair<QString, FileInfo>>)));
    q->connect(fileInfoGatherer, SIGNAL(nameResolved(QString, QString)), q, SLOT(_q_resolvedName(QString, QString)));
    q->connect(fileInfoGatherer, SIGNAL(directoryLoaded(QString)), q, SIGNAL(directoryLoaded(QString)));
#endif // !QT_NO_FILESYSTEMWATCHER
    q->connect(&delayedSortTimer, SIGNAL(timeout()), q, SLOT(_q_performDelayedSort()), Qt::QueuedConnection);
}

/*!
    \internal

    Returns \c false if node doesn't pass the filters otherwise true

    QDir::Modified is not supported
    QDir::Drives is not supported
*/
bool RemoteFileSystemModelPrivate::filtersAcceptsNode(const RemoteFileSystemNode *node) const
{
    Q_Q(const RemoteFileSystemModel);

    if (bypassFilters.contains(node)) {
        qDebug() << "filter bypass:" << node->parent->fileName << "/" << node->fileName;
        return true;
    }

#if 0
    // If we don't know anything yet don't accept it
    if (!node->hasInformation()) {
        qDebug() << "no information:" << node->parent->fileName << "/" << node->fileName;
        return false;
    }
#endif

    bool isDot = (node->fileName == QLatin1String("."));
    bool isDotDot = (node->fileName == QLatin1String(".."));

    // always accept drives
    if (node->parent == &root) {
        if (isDot || isDotDot)
            return false;
        if (q->isWindows()) {
            qDebug() << "accept drive:" << node->fileName;
            return true;
        } else {
            return node->fileName == "/";
        }
    }

    if (!node->exists) {
        qDebug() << "does not exist:" << node->parent->fileName << "/" << node->fileName;
        return false;
    }

    const bool filterPermissions =
        ((filters & QDir::PermissionMask) && (filters & QDir::PermissionMask) != QDir::PermissionMask);
    const bool hideDirs = !(filters & (QDir::Dirs | QDir::AllDirs));
    const bool hideFiles = !nameFilterDisables && !(filters & QDir::Files);
    const bool hideReadable = !(!filterPermissions || (filters & QDir::Readable));
    const bool hideWritable = !(!filterPermissions || (filters & QDir::Writable));
    const bool hideExecutable = !(!filterPermissions || (filters & QDir::Executable));
    const bool hideHidden = !(filters & QDir::Hidden);
    const bool hideSystem = !(filters & QDir::System);
    const bool hideSymlinks = (filters & QDir::NoSymLinks);
    const bool hideDot = (filters & QDir::NoDot);
    const bool hideDotDot = (filters & QDir::NoDotDot);

    // Note that we match the behavior of entryList and not QFileInfo on this.
    if ((hideHidden && !(isDot || isDotDot) && node->isHidden()) || (hideSystem && node->isSystem()) ||
        (hideDirs && node->isDir()) || (hideFiles && node->isFile()) || (hideSymlinks && node->isSymLink()) ||
        (hideReadable && node->isReadable()) || (hideWritable && node->isWritable()) ||
        (hideExecutable && node->isExecutable()) || (hideDot && isDot) || (hideDotDot && isDotDot))
        return false;

    return nameFilterDisables || passNameFilters(node);
}

/*
    \internal

    Returns \c true if node passes the name filters and should be visible.
 */
bool RemoteFileSystemModelPrivate::passNameFilters(const RemoteFileSystemNode *node) const
{
#ifndef QT_NO_REGEXP
    if (nameFilters.isEmpty())
        return true;

    // Check the name regularexpression filters
    if (!(node->isDir() && (filters & QDir::AllDirs))) {
        for (const auto &nameFilter: nameFilters) {
            QRegularExpression copy = nameFilter;
            if (copy.match(node->fileName).hasMatch())
                return true;
        }
        return false;
    }
#endif
    return true;
}

QT_END_NAMESPACE

#include "moc_remotefilesystemmodel.cpp"
