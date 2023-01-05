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
#include <vistle/core/filequery.h>

#include <qdebug.h>
#include <qdiriterator.h>
#include <qstyle.h>
#include <qapplication.h>
#include <qglobal.h>
#ifndef Q_OS_WIN
#include <unistd.h>
#include <sys/types.h>
#endif
#if defined(Q_OS_VXWORKS)
#include "qplatformdefs.h"
#endif

QT_BEGIN_NAMESPACE

#define CERR std::cerr << "VistleFileInfoGatherer: "

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
: AbstractFileInfoGatherer(parent), m_ui(ui), m_moduleId(id)
{
    m_ui->registerFileBrowser(this);
    //start(LowPriority);

    auto &state = m_ui->state();
    int hub = state.getHub(id);
    m_identifier = QString::fromStdString(state.hubName(hub));

    using namespace vistle::message;
    FileQuery fq(m_moduleId, "", FileQuery::SystemInfo);
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

bool VistleFileInfoGatherer::handleMessage(const vistle::message::Message &msg, const vistle::buffer &payload)
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
        m_hostname = QString::fromStdString(info.hostname);
        m_homePath = QString::fromStdString(info.homepath);
        m_workingDirectory = QString::fromStdString(info.currentdir);
        m_userName = QString::fromStdString(info.username);
        emit initialized();
        break;
    }
    case FileQuery::ReadDirectory: {
        QString path = QString::fromStdString(mm.path());

        auto files = vistle::unpackFileList(payload);
        CERR << "ReadDirectory result for " << mm.path() << ": " << files.size() << " entries" << std::endl;

        QStringList allFiles;
        for (auto &f: files) {
            allFiles.push_back(QString::fromStdString(f));
        }

#if 1
        QStringList filelist;
        for (auto &f: allFiles) {
            filelist.push_back(f);
            if (filelist.size() >= 100) {
                fetchExtendedInformation(path, filelist);
                filelist.clear();
            }
        }
        if (!filelist.empty()) {
            fetchExtendedInformation(path, filelist);
            filelist.clear();
        }
#endif

        if (!allFiles.isEmpty())
            emit newListOfFiles(path, allFiles);

        auto it = m_dirs.find(QString::fromStdString(mm.path()));
        if (it == m_dirs.end()) {
            CERR << "ReadDirectory result for unknown directory query " << mm.path() << std::endl;
        } else {
            m_dirs.erase(it);
            if (allFiles.isEmpty())
                emit directoryLoaded(path);
        }
        if (!m_dirs.isEmpty()) {
            qInfo() << "DIRS still waiting" << m_dirs;
        }

        break;
    }
    case FileQuery::LookUpFiles: {
        if (mm.status() != FileQueryResult::Ok) {
            CERR << "LookUpFiles result for '" << mm.path() << "': status not Ok: " << mm.status() << std::endl;
            break;
        }

        auto fivec = vistle::unpackFileInfos(payload);
        CERR << "LookUpFiles result for " << mm.path() << ": " << fivec.size() << " entries" << std::endl;
        std::string path(mm.path());
        if (!path.empty() && path.back() != '/')
            path += "/";

        QVector<QPair<QString, FileInfo>> results;

        auto &qfiles = m_files[QString::fromStdString(mm.path())];

        QStringList allFiles;
        for (auto &f: fivec) {
            FileInfo fi(QDir::cleanPath(QString::fromStdString(path + f.name)));
            fi.m_valid = f.status;
            if (!f.status) {
                CERR << "invalid status for file " << f.name << std::endl;
            }
            fi.m_exists = f.exists;
            if (!f.exists) {
                CERR << "LookUpFiles result for non-existing file " << f.name << std::endl;
            }
            fi.m_isSymlink = f.symlink;
            fi.m_hidden = f.hidden;
            fi.m_size = f.size;
            fi.m_type = FileInfo::System;
            if (f.type == vistle::FileInfo::Directory)
                fi.m_type = FileInfo::Dir;
            if (f.type == vistle::FileInfo::File)
                fi.m_type = FileInfo::File;
            fi.m_lastModified = QDateTime::fromSecsSinceEpoch(f.lastmod);

            fi.m_permissions = QFile::Permissions();
            if (f.permissions & 0400)
                fi.m_permissions |= QFile::Permission::ReadOwner;
            if (f.permissions & 0200)
                fi.m_permissions |= QFile::Permission::WriteOwner;
            if (f.permissions & 0100)
                fi.m_permissions |= QFile::Permission::ExeOwner;
            if (f.permissions & 040)
                fi.m_permissions |= QFile::Permission::ReadGroup;
            if (f.permissions & 020)
                fi.m_permissions |= QFile::Permission::WriteGroup;
            if (f.permissions & 010)
                fi.m_permissions |= QFile::Permission::ExeGroup;
            if (f.permissions & 04)
                fi.m_permissions |= QFile::Permission::ReadOther;
            if (f.permissions & 02)
                fi.m_permissions |= QFile::Permission::WriteOther;
            if (f.permissions & 01)
                fi.m_permissions |= QFile::Permission::ExeOther;

            fi.updateType();
            fi.icon = iconProvider()->icon(fi);
            fi.displayType = iconProvider()->type(fi);

            QString name = QString::fromStdString(f.name);
            results.append(QPair<QString, FileInfo>(name, fi));

            std::string file = path + f.name;
            auto it = qfiles.find(QString::fromStdString(f.name));
            if (it == qfiles.end()) {
                CERR << "LookUpFiles result for unknown file query " << file << std::endl;
                qInfo() << "STILL WAITING" << qfiles;
            } else {
                qfiles.erase(it);
            }
        }

        emit updates(QString::fromStdString(mm.path()), results);
        if (qfiles.empty()) {
            emit directoryLoaded(QString::fromStdString(mm.path()));
            auto it = m_files.find(QString::fromStdString(mm.path()));
            if (it != m_files.end())
                m_files.erase(it);
        } else {
            //qInfo() << "STILL WAITING" << qfiles;
        }

        break;
    }
    case FileQuery::MakeDirectory: {
        if (mm.status() != FileQueryResult::Ok)
            CERR << "making directory " << mm.path() << " failed" << std::endl;
        break;
    }
    }

    return true;
}

QString VistleFileInfoGatherer::identifier() const
{
    return m_identifier;
}

bool VistleFileInfoGatherer::isRootDir(const QString &path) const
{
    return QDir(path).isRoot();
}

QString VistleFileInfoGatherer::hostname() const
{
    //Q_ASSERT(m_initialized);
    return m_hostname;
}

QString VistleFileInfoGatherer::homePath() const
{
    //Q_ASSERT(m_initialized);
    return m_homePath;
}

QString VistleFileInfoGatherer::userName() const
{
    return m_userName;
}

/*!
    Fetch extended information for all \a files in \a path

    \sa updateFile(), update(), resolvedName()
*/
void VistleFileInfoGatherer::fetchExtendedInformation(const QString &path, const QStringList &files)
{
    using namespace vistle::message;
    QString cleanPath = QDir::cleanPath(path);

    if (files.empty()) {
        if (m_dirs.find(cleanPath) == m_dirs.end()) {
            qInfo() << "directory query for" << cleanPath;
            m_dirs.insert(cleanPath);
            vistle::message::FileQuery query(m_moduleId, cleanPath.toStdString(), FileQuery::ReadDirectory);
            sendMessage(query);
        }
    } else {
        QString p = cleanPath;
        if (!p.isEmpty() && !p.endsWith('/'))
            p += "/";
        std::vector<std::string> filelist;
        for (auto f: files) {
            while (f.length() > 1 && f.startsWith('/'))
                f = f.mid(1);
            if (f.isEmpty())
                f += "/";
            auto &qfiles = m_files[cleanPath];
            if (qfiles.find(f) == qfiles.end()) {
                qfiles.insert(f);
                filelist.push_back(f.toStdString());
            }
        }
        if (!filelist.empty()) {
            qInfo() << "file metadata query for" << cleanPath << ":" << filelist.size() << "files";
            auto payload = vistle::packFileList(filelist);
            vistle::message::FileQuery query(m_moduleId, cleanPath.toStdString(), FileQuery::LookUpFiles,
                                             payload.size());
            sendMessage(query, &payload);
        }
    }
}

/*!
    Fetch extended information for all \a filePath

    \sa fetchExtendedInformation()
*/
void VistleFileInfoGatherer::updateFile(const QString &filePath)
{
    QString p = QDir::cleanPath(filePath);
    QString dir = p.mid(0, p.lastIndexOf(QLatin1Char('/')));
    QString fileName = p.mid(dir.length() + 1);
    qInfo() << "updateFile(" << dir << "/" << fileName << ")";
    fetchExtendedInformation(dir, QStringList(fileName));
}

/*
    Remove a \a path from the watcher

    \sa listed()
*/
void VistleFileInfoGatherer::removePath(const QString &path)
{}

FileInfo VistleFileInfoGatherer::getInfo(const QString &path)
{
    FileInfo info;
    QString p = QDir::cleanPath(path);
    if (path.isEmpty())
        info.icon = m_iconProvider->icon(RemoteFileIconProvider::Computer);
    else if (p == homePath())
        info.icon = qApp->style()->standardIcon(QStyle::SP_DirHomeIcon);
    else
        info.icon = m_iconProvider->icon(RemoteFileIconProvider::Network);

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

QString VistleFileInfoGatherer::workingDirectory() const
{
    return m_workingDirectory;
}

QT_END_NAMESPACE

#include "moc_vistlefileinfogatherer.cpp"
