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

#ifndef REMOTEFILEINFOGATHERER_H
#define REMOTEFILEINFOGATHERER_H

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

#include "abstractfileinfogatherer.h"
#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <qfilesystemwatcher.h>
#include <qpair.h>
#include <qstack.h>
#include <qdatetime.h>
#include <qdir.h>
#include <qelapsedtimer.h>

QT_REQUIRE_CONFIG(filesystemmodel);

QT_BEGIN_NAMESPACE

class RemoteFileIconProvider;

class Q_AUTOTEST_EXPORT RemoteFileInfoGatherer: public AbstractFileInfoGatherer {
    Q_OBJECT

public:
    explicit RemoteFileInfoGatherer(QObject *parent = 0);
    ~RemoteFileInfoGatherer() override;

#if QT_CONFIG(filesystemwatcher) && defined(Q_OS_WIN)
    QStringList watchedFiles() const
    {
        return watcher->files();
    }
    QStringList watchedDirectories() const
    {
        return watcher->directories();
    }
    void watchPaths(const QStringList &paths)
    {
        watcher->addPaths(paths);
    }
    void unwatchPaths(const QStringList &paths)
    {
        watcher->removePaths(paths);
    }
#endif // filesystemwatcher && Q_OS_WIN

    QString identifier() const override;
    bool isRootDir(const QString &path) const override;
    QString userName() const override;
    QString homePath() const override;
    QString workingDirectory() const override;

    // only callable from this->thread():
    void clear();
    void removePath(const QString &path) override;
    FileInfo getInfo(const QString &path) override;
    bool isWindows() const override;
    bool mkdir(const QString &path) override;

public Q_SLOTS:
    void fetchExtendedInformation(const QString &path, const QStringList &files) override;
    void updateFile(const QString &path) override;

private Q_SLOTS:
    void driveAdded();
    void driveRemoved();

private:
    void run() override;
    // called by run():
    void getFileInfos(const QString &path, const QStringList &files);
    void fetch(const QFileInfo &info, QElapsedTimer &base, bool &firstTime,
               QVector<QPair<QString, FileInfo>> &updatedFiles, const QString &path);

private:
    mutable QMutex mutex;
    // begin protected by mutex
    QWaitCondition condition;
    QStack<QString> path;
    QStack<QStringList> files;
    // end protected by mutex
    QAtomicInt abort;

#ifndef QT_NO_FILESYSTEMWATCHER
    QFileSystemWatcher *watcher;
#endif
};

QT_END_NAMESPACE
#endif // QFILEINFOGATHERER_H
