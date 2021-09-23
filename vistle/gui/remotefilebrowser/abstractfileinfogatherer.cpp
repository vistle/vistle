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

#include "abstractfileinfogatherer.h"
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

/*!
    Creates thread
*/
AbstractFileInfoGatherer::AbstractFileInfoGatherer(QObject *parent)
: QThread(parent)
,
#ifdef Q_OS_WIN
m_resolveSymlinks(true)
,
#endif
m_iconProvider(&defaultProvider)
{}

/*!
    Destroys thread
*/
AbstractFileInfoGatherer::~AbstractFileInfoGatherer()
{
#if 0
    abort.store(true);
    QMutexLocker locker(&mutex);
    condition.wakeAll();
    locker.unlock();
#endif
    wait();
}

void AbstractFileInfoGatherer::setResolveSymlinks(bool enable)
{
    Q_UNUSED(enable);
#ifdef Q_OS_WIN
    m_resolveSymlinks = enable;
#endif
}

bool AbstractFileInfoGatherer::resolveSymlinks() const
{
#ifdef Q_OS_WIN
    return m_resolveSymlinks;
#else
    return false;
#endif
}

void AbstractFileInfoGatherer::setIconProvider(RemoteFileIconProvider *provider)
{
    m_iconProvider = provider;
}

RemoteFileIconProvider *AbstractFileInfoGatherer::iconProvider() const
{
    return m_iconProvider;
}


/*
    List all files in \a directoryPath

    \sa listed()
*/
void AbstractFileInfoGatherer::list(const QString &directoryPath)
{
    fetchExtendedInformation(directoryPath, QStringList());
}


QT_END_NAMESPACE

#include "moc_abstractfileinfogatherer.cpp"

void FileInfo::updateType()
{
    bool link = isSymLink();
    if (type() == FileInfo::Dir) {
        displayType = "Directory";
        if (link)
            displayType = "Directory (link)";
    } else if (type() == FileInfo::File) {
        displayType = "File";
        if (link)
            displayType = "File (link)";
    } else if (type() == FileInfo::System) {
        displayType = "System";
    } else if (!exists()) {
        displayType = "Non-existent";
    } else {
        displayType = "Unknown";
    }
}
