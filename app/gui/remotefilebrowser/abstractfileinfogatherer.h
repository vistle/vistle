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

#ifndef ABSTRACTFILEINFOGATHERER_H
#define ABSTRACTFILEINFOGATHERER_H

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

#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include "remotefileiconprovider.h"
#include <qpair.h>
#include <qstack.h>
#include <qdatetime.h>
#include <qdir.h>
#include <qelapsedtimer.h>

//#include <private/qfilesystemengine_p.h>

QT_REQUIRE_CONFIG(filesystemmodel);

QT_BEGIN_NAMESPACE

class FileInfo {
public:
    enum Type { Unknown, Dir, File, System };

    FileInfo(const QString &path = QString())
    : m_exists(false), m_isSymlink(false), m_type(Unknown), m_hidden(false), m_size(-1)
    {}

    inline bool exists() { return m_exists; }
    inline bool isDir() { return type() == Dir; }
    inline bool isFile() { return type() == File; }
    inline bool isSystem() { return type() == System; }

    bool operator==(const FileInfo &fi) const
    {
        return fi.m_valid && m_valid && fi.m_permissions == m_permissions && fi.m_isSymlink == m_isSymlink &&
               fi.m_type == m_type && fi.m_hidden == m_hidden && fi.m_size == m_size &&
               fi.m_lastModified == m_lastModified;
    }

#ifndef QT_NO_FSFILEENGINE
    bool isCaseSensitive() const { return m_caseSensitive; }
#endif

    QFile::Permissions permissions() const { return m_permissions; }

    Type type() const { return m_type; }

    bool isSymLink(bool ignoreNtfsSymLinks = false) const
    {
        if (ignoreNtfsSymLinks) {
#ifdef Q_OS_WIN
            //return !mFileInfo.suffix().compare(QLatin1String("lnk"), Qt::CaseInsensitive);
#endif
        }
        return m_isSymlink;
    }

    bool isHidden() const { return m_hidden; }


    QDateTime lastModified() const { return m_lastModified; }

    qint64 size() const { return m_size; }

    void updateType();

    QString displayType = QString("Unknown");
    QIcon icon;

public:
    bool m_valid = false;
    bool m_exists = false;
    QFile::Permissions m_permissions = QFile::Permissions();
    bool m_isSymlink = false;
    Type m_type = Unknown;
    bool m_hidden = false;
    qint64 m_size = -1;
    QDateTime m_lastModified;
    bool m_caseSensitive = true;
    QString m_linkTarget;
};

Q_DECLARE_METATYPE(FileInfo)

class RemoteFileIconProvider;

class Q_AUTOTEST_EXPORT AbstractFileInfoGatherer: public QThread {
    Q_OBJECT

Q_SIGNALS:
    void initialized() const;
    void updates(const QString &directory, const QVector<QPair<QString, FileInfo>> &updates);
    void newListOfFiles(const QString &directory, const QStringList &listOfFiles) const;
    void nameResolved(const QString &fileName, const QString &resolvedName) const;
    void directoryLoaded(const QString &path);

public:
    explicit AbstractFileInfoGatherer(QObject *parent = 0);
    ~AbstractFileInfoGatherer() override;

    virtual QString identifier() const = 0;
    virtual bool isRootDir(const QString &path) const = 0;
    virtual QString userName() const = 0;
    virtual QString homePath() const = 0;
    virtual QString workingDirectory() const = 0;
    virtual QString hostname() const { return ""; }

    // only callable from this->thread():
    virtual void removePath(const QString &path) = 0;
    virtual FileInfo getInfo(const QString &path) = 0;
    RemoteFileIconProvider *iconProvider() const;
    bool resolveSymlinks() const;
    virtual bool isWindows() const = 0;
    virtual bool mkdir(const QString &path) = 0;

public Q_SLOTS:
    virtual void list(const QString &directoryPath);
    virtual void fetchExtendedInformation(const QString &path, const QStringList &files) = 0;
    virtual void updateFile(const QString &path) = 0;
    virtual void setResolveSymlinks(bool enable);
    virtual void setIconProvider(RemoteFileIconProvider *provider);

protected:
#ifdef Q_OS_WIN
    bool m_resolveSymlinks; // not accessed by run()
#endif
    RemoteFileIconProvider *m_iconProvider; // not accessed by run()
    RemoteFileIconProvider defaultProvider;
};

QT_END_NAMESPACE
#endif // QFILEINFOGATHERER_H
