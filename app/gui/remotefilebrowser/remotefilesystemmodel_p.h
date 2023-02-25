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

#ifndef REMOTEFILESYSTEMMODEL_P_H
#define REMOTEFILESYSTEMMODEL_P_H

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

#include "remotefilesystemmodel.h"

#include <qabstractitemmodel.h>
#include "remotefileinfogatherer.h"
#include <qpair.h>
#include <qdir.h>
#include <qicon.h>
#include <qfileinfo.h>
#include <qtimer.h>
#include <qhash.h>
#include <qstyle.h>
#include <qapplication.h>

QT_REQUIRE_CONFIG(filesystemmodel);

QT_BEGIN_NAMESPACE

class ExtendedInformation;
class RemoteFileSystemModelPrivate;
class RemoteFileIconProvider;

class RemoteFileSystemModelNodePathKey: public QString {
public:
    RemoteFileSystemModelNodePathKey() {}
    RemoteFileSystemModelNodePathKey(const QString &other, Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive)
    : QString(other), caseSensitivity(caseSensitivity)
    {}
    RemoteFileSystemModelNodePathKey(const RemoteFileSystemModelNodePathKey &other)
    : QString(other), caseSensitivity(other.caseSensitivity)
    {}
    bool operator==(const RemoteFileSystemModelNodePathKey &other) const
    {
        if (caseSensitivity == Qt::CaseSensitive && other.caseSensitivity == Qt::CaseSensitive)
            return !compare(other, Qt::CaseSensitive);
        return !compare(other, Qt::CaseInsensitive);
    }

    Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
};

Q_DECLARE_TYPEINFO(RemoteFileSystemModelNodePathKey, Q_MOVABLE_TYPE);

inline uint qHash(const RemoteFileSystemModelNodePathKey &key)
{
    if (key.caseSensitivity == Qt::CaseSensitive)
        return qHash(static_cast<QString>(key));
    return qHash(key.toCaseFolded());
}

class Q_AUTOTEST_EXPORT RemoteFileSystemModelPrivate // : public QAbstractItemModelPrivate
{
    class RemoteFileSystemModel *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(RemoteFileSystemModel)

public:
    enum { NumColumns = 4 };

    class RemoteFileSystemNode {
    public:
        explicit RemoteFileSystemNode(RemoteFileSystemModel *model, const QString &filename = QString(),
                                      RemoteFileSystemNode *p = 0)
        : fileName(filename)
        , populatedChildren(false)
        , m_isVisible(false)
        , dirtyChildrenIndex(-1)
        , parent(p)
        , info(0)
        , model(model)
        {}
        ~RemoteFileSystemNode()
        {
            qDeleteAll(children);
            delete info;
            info = 0;
            parent = 0;
        }

        QString fileName;
        QString volumeName;

        inline qint64 size() const
        {
            if (info && !info->isDir())
                return info->size();
            return 0;
        }
        inline QString type() const
        {
            if (info)
                return info->displayType;
            return QLatin1String("");
        }
        inline QDateTime lastModified() const
        {
            if (info)
                return info->lastModified();
            return QDateTime();
        }
        inline QFile::Permissions permissions() const
        {
            if (info)
                return info->permissions();
            return QFile::Permissions();
        }
        inline bool isReadable() const { return ((permissions() & QFile::ReadUser) != 0); }
        inline bool isWritable() const { return ((permissions() & QFile::WriteUser) != 0); }
        inline bool isExecutable() const { return ((permissions() & QFile::ExeUser) != 0); }
        inline bool isDir() const
        {
            if (info)
                return info->isDir();
            if (children.count() > 0)
                return true;
            return false;
        }
        inline FileInfo fileInfo() const
        {
            if (info)
                return *info;
            return FileInfo();
        }
        inline bool isFile() const
        {
            if (info)
                return info->isFile();
            return true;
        }
        inline bool isSystem() const
        {
            if (info)
                return info->isSystem();
            return true;
        }
        inline bool isHidden() const
        {
            if (info)
                return info->isHidden();
            return false;
        }
        inline bool isSymLink(bool ignoreNtfsSymLinks = false) const
        {
            return info && info->isSymLink(ignoreNtfsSymLinks);
        }
        inline bool caseSensitive() const
        {
            if (info)
                return info->isCaseSensitive();
            return false;
        }
        inline QIcon icon() const
        {
            if (info)
                return info->icon;
            return QIcon();
        }
        QString linkTarget() const
        {
            if (info)
                return info->m_linkTarget;
            return QString();
        }

        inline bool operator<(const RemoteFileSystemNode &node) const
        {
            if (caseSensitive() || node.caseSensitive())
                return fileName < node.fileName;
            return QString::compare(fileName, node.fileName, Qt::CaseInsensitive) < 0;
        }
        inline bool operator>(const QString &name) const
        {
            if (caseSensitive())
                return fileName > name;
            return QString::compare(fileName, name, Qt::CaseInsensitive) > 0;
        }
        inline bool operator<(const QString &name) const
        {
            if (caseSensitive())
                return fileName < name;
            return QString::compare(fileName, name, Qt::CaseInsensitive) < 0;
        }
        inline bool operator!=(const FileInfo &fileInfo) const { return !operator==(fileInfo); }
        bool operator==(const QString &name) const
        {
            if (caseSensitive())
                return fileName == name;
            return QString::compare(fileName, name, Qt::CaseInsensitive) == 0;
        }
        bool operator==(const FileInfo &fileInfo) const { return info && (*info == fileInfo); }

        inline bool hasInformation() const { return info != 0 && info->m_valid; }

        inline bool isVisible() const { return m_isVisible; /* return hasInformation() && m_isVisible && exists; */ }

        void populate(const FileInfo &fileInfo)
        {
            if (!info)
                info = new FileInfo(fileInfo);
            (*info) = fileInfo;
            exists = info->exists();
        }

        // children shouldn't normally be accessed directly, use node()
        inline int visibleLocation(const QString &childName) { return visibleChildren.indexOf(childName); }

        void updateIcon(RemoteFileIconProvider *iconProvider, const QString &path)
        {
            if (info) {
                if (info->type() == FileInfo::Dir) {
                    info->icon = iconProvider->icon(RemoteFileIconProvider::Folder);
                    if (model->homePath() == path)
                        info->icon = qApp->style()->standardIcon(QStyle::SP_DirHomeIcon);
                } else if (info->type() == FileInfo::File) {
                    info->icon = iconProvider->icon(RemoteFileIconProvider::File);
                } else if (info->type() == FileInfo::System) {
                    info->icon = iconProvider->icon(RemoteFileIconProvider::Drive);
                } else if (!info->exists()) {
                    info->icon = iconProvider->icon(RemoteFileIconProvider::Trashcan);
                } else {
                    info->icon = iconProvider->icon(RemoteFileIconProvider::Network);
                }
            }
            for (RemoteFileSystemNode *child: qAsConst(children)) {
                //On windows the root (My computer) has no path so we don't want to add a / for nothing (e.g. /C:/)
                if (!path.isEmpty()) {
                    if (path.endsWith(QLatin1Char('/')))
                        child->updateIcon(iconProvider, path + child->fileName);
                    else
                        child->updateIcon(iconProvider, path + QLatin1Char('/') + child->fileName);
                } else
                    child->updateIcon(iconProvider, child->fileName);
            }
        }

        void retranslateStrings(RemoteFileIconProvider *iconProvider, const QString &path)
        {
            if (info) {
                info->updateType();
            }
            for (RemoteFileSystemNode *child: qAsConst(children)) {
                //On windows the root (My computer) has no path so we don't want to add a / for nothing (e.g. /C:/)
                if (!path.isEmpty()) {
                    if (path.endsWith(QLatin1Char('/')))
                        child->retranslateStrings(iconProvider, path + child->fileName);
                    else
                        child->retranslateStrings(iconProvider, path + QLatin1Char('/') + child->fileName);
                } else
                    child->retranslateStrings(iconProvider, child->fileName);
            }
        }

        bool populatedChildren;
        bool exists = false;
        bool m_isVisible = false;
        QHash<RemoteFileSystemModelNodePathKey, RemoteFileSystemNode *> children;
        QList<QString> visibleChildren;
        int dirtyChildrenIndex;
        RemoteFileSystemNode *parent;
        QString m_linkTarget;

        FileInfo *info = nullptr;
        RemoteFileSystemModel *model = nullptr;
    };

    RemoteFileSystemModelPrivate(RemoteFileSystemModel *q, AbstractFileInfoGatherer *fileinfogatherer)
    : q_ptr(q)
    , fileInfoGatherer(fileinfogatherer)
    , forceSort(true)
    , sortColumn(0)
    , sortOrder(Qt::AscendingOrder)
    , readOnly(true)
    , setRootPath(false)
    , filters(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs)
    , nameFilterDisables(true)
    , // false on windows, true on mac and unix
    disableRecursiveSort(false)
    , root(q)
    {
        delayedSortTimer.setSingleShot(true);
    }

    void init();
    /*
      \internal

      Return true if index which is owned by node is hidden by the filter.
    */
    inline bool isHiddenByFilter(RemoteFileSystemNode *indexNode, const QModelIndex &index) const
    {
        return (indexNode != &root && !index.isValid());
    }
    RemoteFileSystemNode *node(const QModelIndex &index) const;
    RemoteFileSystemNode *node(const QString &path, bool fetch = true) const;
    inline QModelIndex index(const QString &path, int column = 0) { return index(node(path), column); }
    QModelIndex index(const RemoteFileSystemNode *node, int column = 0) const;
    bool filtersAcceptsNode(const RemoteFileSystemNode *node) const;
    bool passNameFilters(const RemoteFileSystemNode *node) const;
    void removeNode(RemoteFileSystemNode *parentNode, const QString &name);
    RemoteFileSystemNode *addNode(RemoteFileSystemNode *parentNode, const QString &fileName, const FileInfo &info);
    void addVisibleFiles(RemoteFileSystemNode *parentNode, const QStringList &newFiles);
    void removeVisibleFile(RemoteFileSystemNode *parentNode, int visibleLocation);
    void sortChildren(int column, const QModelIndex &parent);

    inline int translateVisibleLocation(RemoteFileSystemNode *parent, int row) const
    {
        if (sortOrder != Qt::AscendingOrder) {
            if (parent->dirtyChildrenIndex == -1)
                return parent->visibleChildren.count() - row - 1;

            if (row < parent->dirtyChildrenIndex)
                return parent->dirtyChildrenIndex - row - 1;
        }

        return row;
    }

    inline static QString myComputer()
    {
        // ### TODO We should query the system to find out what the string should be
        // XP == "My Computer",
        // Vista == "Computer",
        // OS X == "Computer" (sometime user generated) "Benjamin's PowerBook G4"
#ifdef Q_OS_WIN
        return RemoteFileSystemModel::tr("My Computer");
#else
        return RemoteFileSystemModel::tr("Computer");
#endif
    }

    inline void delayedSort()
    {
        if (!delayedSortTimer.isActive())
            delayedSortTimer.start(0);
    }

    QIcon icon(const QModelIndex &index) const;
    QString name(const QModelIndex &index) const;
    QString displayName(const QModelIndex &index) const;
    QString filePath(const QModelIndex &index) const;
    QString size(const QModelIndex &index) const;
    static QString size(qint64 bytes);
    QString type(const QModelIndex &index) const;
    QString time(const QModelIndex &index) const;

    void _q_modelInitialized();
    void _q_directoryChanged(const QString &directory, const QStringList &list);
    void _q_performDelayedSort();
    void _q_fileSystemChanged(const QString &path, const QVector<QPair<QString, FileInfo>> &);
    void _q_resolvedName(const QString &fileName, const QString &resolvedName);

    static int naturalCompare(const QString &s1, const QString &s2, Qt::CaseSensitivity cs);

    //QDir rootDir;
    QString rootPath;
#if QT_CONFIG(filesystemwatcher)
#ifdef Q_OS_WIN
    //QStringList unwatchPathsAt(const QModelIndex &);
    //void watchPaths(const QStringList &paths) { fileInfoGatherer->watchPaths(paths); }
#endif // Q_OS_WIN
    AbstractFileInfoGatherer *fileInfoGatherer;
#endif // filesystemwatcher
    QTimer delayedSortTimer;
    bool forceSort;
    int sortColumn;
    Qt::SortOrder sortOrder;
    bool readOnly;
    bool setRootPath;
    QDir::Filters filters;
    QHash<const RemoteFileSystemNode *, bool> bypassFilters;
    bool nameFilterDisables;
    //This flag is an optimization for the RemoteFileDialog
    //It enable a sort which is not recursive, it means
    //we sort only what we see.
    bool disableRecursiveSort;
#ifndef QT_NO_REGEXP
    QList<QRegularExpression> nameFilters;
#endif
    QHash<QString, QString> resolvedSymLinks;

    RemoteFileSystemNode root;

    QBasicTimer fetchingTimer;
    struct Fetching {
        QString dir;
        QString file;
        QModelIndex nodeIndex;
        //const RemoteFileSystemNode *node = nullptr;
    };
    QVector<Fetching> toFetch;
    bool fileinfoGathererInitialized = false;
};
Q_DECLARE_TYPEINFO(RemoteFileSystemModelPrivate::Fetching, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif
