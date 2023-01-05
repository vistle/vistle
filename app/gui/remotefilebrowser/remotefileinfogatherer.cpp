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

#include "remotefileinfogatherer.h"
#include <qdebug.h>
#include <qdiriterator.h>
#ifndef Q_OS_WIN
#include <unistd.h>
#include <sys/types.h>
#endif
#if defined(Q_OS_VXWORKS)
#include "qplatformdefs.h"
#endif

QT_BEGIN_NAMESPACE

#ifdef QT_BUILD_INTERNAL
static QBasicAtomicInt fetchedRoot = Q_BASIC_ATOMIC_INITIALIZER(false);
Q_AUTOTEST_EXPORT void qt_test_resetFetchedRoot()
{
    fetchedRoot.store(false);
}

Q_AUTOTEST_EXPORT bool qt_test_isFetchedRoot()
{
    return fetchedRoot.load();
}
#endif

static QString translateDriveName(const QFileInfo &drive)
{
    QString driveName = drive.absoluteFilePath();
#ifdef Q_OS_WIN
    if (driveName.startsWith(QLatin1Char('/'))) // UNC host
        return drive.fileName();
    if (driveName.endsWith(QLatin1Char('/')))
        driveName.chop(1);
#endif // Q_OS_WIN
    return driveName;
}

FileInfo toFileInfo(const QFileInfo &info)
{
    FileInfo fi;

    fi.m_valid = true;
    fi.m_exists = info.exists();
    fi.m_permissions = info.permissions();
    fi.m_isSymlink = info.isSymLink();
    fi.m_type = FileInfo::System;
    fi.m_hidden = info.isHidden();
    fi.m_size = -1;
    fi.m_lastModified = info.lastModified();

    if (info.isDir())
        fi.m_type = FileInfo::Dir;
    else if (info.isFile())
        fi.m_type = FileInfo::File;
    else if (!info.exists() && info.isSymLink())
        fi.m_type = FileInfo::System;

    if (fi.type() == FileInfo::Dir)
        fi.m_size = 0;
    else if (fi.type() == FileInfo::File)
        fi.m_size = info.size();
    if (!info.exists() && !info.isSymLink())
        fi.m_size = -1;

    return fi;
}

/*!
    Creates thread
*/
RemoteFileInfoGatherer::RemoteFileInfoGatherer(QObject *parent)
: AbstractFileInfoGatherer(parent)
, abort(false)
#ifndef QT_NO_FILESYSTEMWATCHER
, watcher(0)
#endif
{
    qDebug() << "new RemoteFileInfoGatherer";
#ifndef QT_NO_FILESYSTEMWATCHER
    watcher = new QFileSystemWatcher(this);
    connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(list(QString)));
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(updateFile(QString)));

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    const QVariant listener = watcher->property("_q_driveListener");
    if (listener.canConvert<QObject *>()) {
        if (QObject *driveListener = listener.value<QObject *>()) {
            connect(driveListener, SIGNAL(driveAdded()), this, SLOT(driveAdded()));
            connect(driveListener, SIGNAL(driveRemoved()), this, SLOT(driveRemoved()));
        }
    }
#endif // Q_OS_WIN && !Q_OS_WINRT
#endif
    start(LowPriority);

    emit initialized();
}

/*!
    Destroys thread
*/
RemoteFileInfoGatherer::~RemoteFileInfoGatherer()
{
    abort.storeRelaxed(true);
    QMutexLocker locker(&mutex);
    condition.wakeAll();
    locker.unlock();
    wait();
}

QString RemoteFileInfoGatherer::identifier() const
{
    return QString("");
}

bool RemoteFileInfoGatherer::isRootDir(const QString &path) const
{
    return QDir(path).isRoot();
}

QString RemoteFileInfoGatherer::userName() const
{
#ifdef Q_OS_WIN
    auto p = getenv("USERNAME");
#else
    auto p = getenv("USER");
#endif
    if (p) {
        return QString::fromStdString(p);
    }
    return QString("unknown");
}

QString RemoteFileInfoGatherer::homePath() const
{
    return QDir::homePath();
}

void RemoteFileInfoGatherer::driveAdded()
{
    fetchExtendedInformation(QString(), QStringList());
}

void RemoteFileInfoGatherer::driveRemoved()
{
    QStringList drives;
    const QFileInfoList driveInfoList = QDir::drives();
    for (const QFileInfo &fi: driveInfoList)
        drives.append(translateDriveName(fi));
    newListOfFiles(QString(), drives);
}

/*!
    Fetch extended information for all \a files in \a path

    \sa updateFile(), update(), resolvedName()
*/
void RemoteFileInfoGatherer::fetchExtendedInformation(const QString &path, const QStringList &files)
{
    QMutexLocker locker(&mutex);
    // See if we already have this dir/file in our queue
    int loc = this->path.lastIndexOf(path);
    while (loc > 0) {
        if (this->files.at(loc) == files) {
            return;
        }
        loc = this->path.lastIndexOf(path, loc - 1);
    }
    this->path.push(path);
    this->files.push(files);
    condition.wakeAll();

#ifndef QT_NO_FILESYSTEMWATCHER
    if (files.isEmpty() && !path.isEmpty() && !path.startsWith(QLatin1String("//")) /*don't watch UNC path*/) {
        if (!watcher->directories().contains(path))
            watcher->addPath(path);
    }
#endif
}

/*!
    Fetch extended information for all \a filePath

    \sa fetchExtendedInformation()
*/
void RemoteFileInfoGatherer::updateFile(const QString &filePath)
{
    QString dir = filePath.mid(0, filePath.lastIndexOf(QLatin1Char('/')));
    QString fileName = filePath.mid(dir.length() + 1);
    fetchExtendedInformation(dir, QStringList(fileName));
}

/*
    List all files in \a directoryPath

    \sa listed()
*/
void RemoteFileInfoGatherer::clear()
{
#ifndef QT_NO_FILESYSTEMWATCHER
    QMutexLocker locker(&mutex);
    watcher->removePaths(watcher->files());
    watcher->removePaths(watcher->directories());
#endif
}

/*
    Remove a \a path from the watcher

    \sa listed()
*/
void RemoteFileInfoGatherer::removePath(const QString &path)
{
#ifndef QT_NO_FILESYSTEMWATCHER
    QMutexLocker locker(&mutex);
    watcher->removePath(path);
#else
    Q_UNUSED(path);
#endif
}

FileInfo RemoteFileInfoGatherer::getInfo(const QString &path)
{
    QFileInfo finfo(path);
    FileInfo info = toFileInfo(finfo);
#if 0
    info.icon = m_iconProvider->icon(path);
    info.displayType = m_iconProvider->type(path);
#endif
#ifndef QT_NO_FILESYSTEMWATCHER
    // ### Not ready to listen all modifications by default
    static const bool watchFiles = qEnvironmentVariableIsSet("QT_FILESYSTEMMODEL_WATCH_FILES");
    if (watchFiles) {
        if (!finfo.exists() && !finfo.isSymLink()) {
            watcher->removePath(finfo.absoluteFilePath());
        } else {
            const QString path = finfo.absoluteFilePath();
            if (!path.isEmpty() && finfo.exists() && finfo.isFile() && finfo.isReadable() &&
                !watcher->files().contains(path)) {
                watcher->addPath(path);
            }
        }
    }
#endif

#ifdef Q_OS_WIN
    if (m_resolveSymlinks && info.isSymLink(/* ignoreNtfsSymLinks = */ true)) {
        QFileInfo resolvedInfo(finfo.symLinkTarget());
        resolvedInfo = QFileInfo(resolvedInfo.canonicalFilePath());
        if (resolvedInfo.exists()) {
            emit nameResolved(finfo.filePath(), resolvedInfo.fileName());
        }
    }
#endif
    return info;
}

bool RemoteFileInfoGatherer::isWindows() const
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

bool RemoteFileInfoGatherer::mkdir(const QString &path)
{
    QFileInfo info(path);
    if (info.exists()) {
        return info.isDir();
    }

    QDir dir(info.absoluteDir());
    return dir.mkdir(info.baseName());
}

/*
    Get specific file info's, batch the files so update when we have 100
    items and every 200ms after that
 */
void RemoteFileInfoGatherer::getFileInfos(const QString &path, const QStringList &files)
{
    // List drives
    if (path.isEmpty()) {
#ifdef QT_BUILD_INTERNAL
        fetchedRoot.store(true);
#endif
        QFileInfoList infoList;
        if (files.isEmpty()) {
            infoList = QDir::drives();
        } else {
            infoList.reserve(files.count());
            for (const auto &file: files)
                infoList << QFileInfo(file);
        }
        for (int i = infoList.count() - 1; i >= 0; --i) {
            QString driveName = translateDriveName(infoList.at(i));
            QVector<QPair<QString, FileInfo>> updatedFiles;
            updatedFiles.append(QPair<QString, FileInfo>(driveName, toFileInfo(infoList.at(i))));
            emit updates(path, updatedFiles);
        }
        return;
    }

    QElapsedTimer base;
    base.start();
    QFileInfo fileInfo;
    bool firstTime = true;
    QVector<QPair<QString, FileInfo>> updatedFiles;
    QStringList filesToCheck = files;

    QStringList allFiles;
    if (files.isEmpty()) {
        QDirIterator dirIt(path, QDir::AllEntries | QDir::System | QDir::Hidden);
        while (!abort.loadRelaxed() && dirIt.hasNext()) {
            dirIt.next();
            fileInfo = dirIt.fileInfo();
            allFiles.append(fileInfo.fileName());
            fetch(fileInfo, base, firstTime, updatedFiles, path);
        }
    }
    if (!allFiles.isEmpty())
        emit newListOfFiles(path, allFiles);

    QStringList::const_iterator filesIt = filesToCheck.constBegin();
    while (!abort.loadRelaxed() && filesIt != filesToCheck.constEnd()) {
        fileInfo.setFile(path + QDir::separator() + *filesIt);
        ++filesIt;
        fetch(fileInfo, base, firstTime, updatedFiles, path);
    }
    if (!updatedFiles.isEmpty())
        emit updates(path, updatedFiles);
    emit directoryLoaded(path);
}

void RemoteFileInfoGatherer::fetch(const QFileInfo &fileInfo, QElapsedTimer &base, bool &firstTime,
                                   QVector<QPair<QString, FileInfo>> &updatedFiles, const QString &path)
{
    updatedFiles.append(QPair<QString, FileInfo>(fileInfo.fileName(), toFileInfo(fileInfo)));
    QElapsedTimer current;
    current.start();
    if ((firstTime && updatedFiles.count() > 100) || base.msecsTo(current) > 1000) {
        emit updates(path, updatedFiles);
        updatedFiles.clear();
        base = current;
        firstTime = false;
    }
}

/*
    Until aborted wait to fetch a directory or files
*/
void RemoteFileInfoGatherer::run()
{
    forever
    {
        QMutexLocker locker(&mutex);
        while (!abort.loadRelaxed() && path.isEmpty())
            condition.wait(&mutex);
        if (abort.loadRelaxed())
            return;
        const QString thisPath = qAsConst(path).front();
        path.pop_front();
        const QStringList thisList = qAsConst(files).front();
        files.pop_front();
        locker.unlock();

        getFileInfos(thisPath, thisList);
    }
}


QString RemoteFileInfoGatherer::workingDirectory() const
{
    return QDir::currentPath();
}

QT_END_NAMESPACE

#include "moc_remotefileinfogatherer.cpp"
