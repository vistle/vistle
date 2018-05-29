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

#include "vistlefileinfogatherer.h"
#include <core/filequery.h>

#include <qdebug.h>
#include <qdiriterator.h>
#ifndef Q_OS_WIN
#  include <unistd.h>
#  include <sys/types.h>
#endif
#if defined(Q_OS_VXWORKS)
#  include "qplatformdefs.h"
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

/*!
    Creates thread
*/
VistleFileInfoGatherer::VistleFileInfoGatherer(vistle::UserInterface *ui, int id, QObject *parent)
    : AbstractFileInfoGatherer(parent)
    , m_ui(ui)
    , m_moduleId(id)
{
    qDebug() << "new VistleFileInfoGatherer";
    m_ui->registerFileBrowser(this);
    //start(LowPriority);

    using namespace vistle::message;
    FileQuery fq(m_moduleId, "/", FileQuery::SystemInfo);
    sendMessage(fq);
}

/*!
    Destroys thread
*/
VistleFileInfoGatherer::~VistleFileInfoGatherer()
{
    m_ui->removeFileBrowser(this);
#if 0
    abort.store(true);
    QMutexLocker locker(&mutex);
    condition.wakeAll();
    locker.unlock();
#endif
    wait();
}

bool VistleFileInfoGatherer::handleMessage(const vistle::message::Message &msg, const std::vector<char> &payload)
{
    using namespace vistle::message;

    if (msg.type() != vistle::message::FILEQUERYRESULT)
        return false;

    auto &mm = msg.as<vistle::message::FileQueryResult>();
    switch (mm.command()) {
    case FileQuery::SystemInfo: {
        m_initialized = true;
        auto info = vistle::unpackSystemInfo(payload);
        m_isWindows = info.iswindows;
        m_homePath = QString::fromStdString(info.homepath);
        m_workingDirectory = QString::fromStdString(info.currentdir);
        break;
    }
    case FileQuery::ReadDirectory: {
        QString path = QString::fromStdString(mm.path());

        auto files = vistle::unpackFileList(payload);
        std::cerr << "result for " << mm.path() << ": " << files.size() << " entries" << std::endl;

        QStringList allFiles;
        for (auto &f: files) {
            allFiles.push_back(QString::fromStdString(f));
        }
        emit newListOfFiles(path, allFiles);
        emit directoryLoaded(QString::fromStdString(mm.path()));

        QStringList filelist;
        for (auto &f: allFiles) {
            filelist.push_back(f);
            if (filelist.size() >= 50) {
                fetchExtendedInformation(path, filelist);
                filelist.clear();
            }
        }
        if (!filelist.empty()) {
            fetchExtendedInformation(path, filelist);
            filelist.clear();
        }

        auto it = m_dirs.find(QString::fromStdString(mm.path()));
        if (it == m_dirs.end()) {
            std::cerr << "VistleFileInfoGatherer: result for unknown directory query " << mm.path() << std::endl;
        } else {
            m_dirs.erase(it);
        }

        break;
    }
    case FileQuery::LookUpFiles: {
        if (mm.status() != FileQueryResult::Ok) {
            std::cerr << "result for " << mm.path() << ": status not Ok: " << mm.status() << std::endl;
            break;
        }

        auto fivec = vistle::unpackFileInfos(payload);
        std::cerr << "result for " << mm.path() << ": " << fivec.size() << " entries" << std::endl;
        std::string path(mm.path());

        QVector<QPair<QString, FileInfo>> results;

        QStringList allFiles;
        for (auto &f: fivec) {
            FileInfo fi;
            fi.m_valid = true;
            fi.m_exists = f.exists;
            if (!f.exists) {
                std::cerr << "result for non-existing file " << f.name << std::endl;
            }
            fi.m_isSymlink = f.symlink;
            fi.m_hidden = f.hidden;
            fi.m_size = f.size;
            fi.m_type = FileInfo::System;
            if (f.type == vistle::FileInfo::Directory)
                fi.m_type = FileInfo::Dir;
            if (f.type == vistle::FileInfo::File)
                fi.m_type = FileInfo::File;
            fi.m_lastModified = QDateTime::fromTime_t(f.lastmod);

            fi.m_permissions = 0;
            if (f.permissions & 0400) fi.m_permissions |= QFile::Permission::ReadOwner;
            if (f.permissions & 0200) fi.m_permissions |= QFile::Permission::WriteOwner;
            if (f.permissions & 0100) fi.m_permissions |= QFile::Permission::ExeOwner;
            if (f.permissions & 040) fi.m_permissions |= QFile::Permission::ReadGroup;
            if (f.permissions & 020) fi.m_permissions |= QFile::Permission::WriteGroup;
            if (f.permissions & 010) fi.m_permissions |= QFile::Permission::ExeGroup;
            if (f.permissions & 04) fi.m_permissions |= QFile::Permission::ReadOther;
            if (f.permissions & 02) fi.m_permissions |= QFile::Permission::WriteOther;
            if (f.permissions & 01) fi.m_permissions |= QFile::Permission::ExeOther;

            fi.updateType();

            QString name = QString::fromStdString(f.name);
            results.append(QPair<QString,FileInfo>(name, fi));

            std::string file = path + "/" + f.name;
            auto it = m_files.find(QString::fromStdString(file));
            if (it == m_files.end()) {
                std::cerr << "VistleFileInfoGatherer: result for unknown file query " << file << std::endl;
            } else {
                m_files.erase(it);
            }
        }

        emit updates(QString::fromStdString(mm.path()), results);
        emit directoryLoaded(QString::fromStdString(mm.path()));

        break;
    }
    case FileQuery::MakeDirectory: {
        if (mm.status() != FileQueryResult::Ok)
            std::cerr << "VistleFileInfoGatherer: making directory " << mm.path() << " failed" << std::endl;
        break;
    }

    }

    return true;
}

bool VistleFileInfoGatherer::isRootDir(const QString &path) const
{
    return QDir(path).isRoot();
}

QString VistleFileInfoGatherer::homePath() const
{
    //Q_ASSERT(m_initialized);
    return m_homePath;
}

/*!
    Fetch extended information for all \a files in \a path

    \sa updateFile(), update(), resolvedName()
*/
void VistleFileInfoGatherer::fetchExtendedInformation(const QString &path, const QStringList &files)
{
    using namespace vistle::message;

    if (files.empty()) {
        if (m_dirs.find(path) == m_dirs.end()) {
            m_dirs.insert(path);
            vistle::message::FileQuery query(m_moduleId, path.toStdString(), FileQuery::ReadDirectory);
            sendMessage(query);
        }
    } else {
        std::vector<std::string> filelist;
        for (auto &f: files) {
            filelist.push_back(f.toStdString());
            QString p = path;
            p += "/";
            p += f;
            if (m_files.find(p) == m_files.end()) {
                m_files.insert(p);
                std::cerr << "file query for " << p.toStdString() << std::endl;
            }
        }
        auto payload = vistle::packFileList(filelist);
        vistle::message::FileQuery query(m_moduleId, path.toStdString(), FileQuery::LookUpFiles, payload.size());
        sendMessage(query, &payload);
    }
}

/*!
    Fetch extended information for all \a filePath

    \sa fetchExtendedInformation()
*/
void VistleFileInfoGatherer::updateFile(const QString &filePath)
{
    QString dir = filePath.mid(0, filePath.lastIndexOf(QLatin1Char('/')));
    QString fileName = filePath.mid(dir.length() + 1);
    fetchExtendedInformation(dir, QStringList(fileName));
}

/*
    Remove a \a path from the watcher

    \sa listed()
*/
void VistleFileInfoGatherer::removePath(const QString &path)
{
}

FileInfo VistleFileInfoGatherer::getInfo(const QString &path)
{
    FileInfo info(path);
    info.m_valid = false;
    info.icon = m_iconProvider->icon(QFileIconProvider::Network);
    info.displayType = "Unknown";

    updateFile(path);

    return info;
}

bool VistleFileInfoGatherer::isWindows() const
{
    //Q_ASSERT(m_initialized);
    return m_isWindows;
}

bool VistleFileInfoGatherer::mkdir(const QString &path)
{
    using namespace vistle::message;
    FileQuery fq(m_moduleId, path.toStdString(), FileQuery::MakeDirectory);
    return sendMessage(fq);
}

#if 0
/*
    Get specific file info's, batch the files so update when we have 100
    items and every 200ms after that
 */
void VistleFileInfoGatherer::getFileInfos(const QString &path, const QStringList &files)
{
#if 0
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
            for (const auto &file : files)
                infoList << QFileInfo(file);
        }
        for (int i = infoList.count() - 1; i >= 0; --i) {
            QString driveName = translateDriveName(infoList.at(i));
            QVector<QPair<QString,FileInfo> > updatedFiles;
            updatedFiles.append(QPair<QString,FileInfo>(driveName, FileInfo(infoList.at(i))));
            emit updates(path, updatedFiles);
        }
        return;
    }

    QElapsedTimer base;
    base.start();
    QFileInfo fileInfo;
    bool firstTime = true;
    QVector<QPair<QString, FileInfo> > updatedFiles;
    QStringList filesToCheck = files;

    QStringList allFiles;
    if (files.isEmpty()) {
        QDirIterator dirIt(path, QDir::AllEntries | QDir::System | QDir::Hidden);
        while (!abort.load() && dirIt.hasNext()) {
            dirIt.next();
            fileInfo = dirIt.fileInfo();
            allFiles.append(fileInfo.fileName());
            fetch(fileInfo, base, firstTime, updatedFiles, path);
        }
    }
    if (!allFiles.isEmpty())
        emit newListOfFiles(path, allFiles);

    QStringList::const_iterator filesIt = filesToCheck.constBegin();
    while (!abort.load() && filesIt != filesToCheck.constEnd()) {
        fileInfo.setFile(path + QDir::separator() + *filesIt);
        ++filesIt;
        fetch(fileInfo, base, firstTime, updatedFiles, path);
    }
    if (!updatedFiles.isEmpty())
        emit updates(path, updatedFiles);
    emit directoryLoaded(path);
#endif
}
#endif

#if 0
void VistleFileInfoGatherer::fetch(const QFileInfo &fileInfo, QElapsedTimer &base, bool &firstTime, QVector<QPair<QString, FileInfo> > &updatedFiles, const QString &path) {
    updatedFiles.append(QPair<QString, FileInfo>(fileInfo.fileName(), fileInfo));
    QElapsedTimer current;
    current.start();
    if ((firstTime && updatedFiles.count() > 100) || base.msecsTo(current) > 1000) {
        emit updates(path, updatedFiles);
        updatedFiles.clear();
        base = current;
        firstTime = false;
    }
}
#endif



QString VistleFileInfoGatherer::workingDirectory() const
{
    return m_workingDirectory;
}

QT_END_NAMESPACE

#include "moc_vistlefileinfogatherer.cpp"
