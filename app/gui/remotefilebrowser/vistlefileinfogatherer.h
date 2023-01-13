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

#ifndef VISTLEFILEINFOGATHERER_H
#define VISTLEFILEINFOGATHERER_H

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
#include <qdatetime.h>
#include <qdir.h>
#include <qelapsedtimer.h>
#include <qfilesystemwatcher.h>
#include <QMap>
#include <qmutex.h>
#include <qpair.h>
#include <QSet>
#include <qstack.h>
#include <qthread.h>
#include <qwaitcondition.h>

#include <vistle/userinterface/userinterface.h>

#include <vistle/util/buffer.h>

QT_REQUIRE_CONFIG(filesystemmodel);

QT_BEGIN_NAMESPACE

class RemoteFileIconProvider;

class Q_AUTOTEST_EXPORT VistleFileInfoGatherer: public AbstractFileInfoGatherer, public vistle::FileBrowser {
    Q_OBJECT

public:
    explicit VistleFileInfoGatherer(vistle::UserInterface *ui, int moduleId, QObject *parent = 0);
    ~VistleFileInfoGatherer() override;

    bool handleMessage(const vistle::message::Message &message, const vistle::buffer &payload) override;

    QString identifier() const override;
    bool isRootDir(const QString &path) const override;
    QString homePath() const override;
    QString userName() const override;
    QString workingDirectory() const override;
    QString hostname() const override;

    // only callable from this->thread():
    void removePath(const QString &path) override;
    FileInfo getInfo(const QString &path) override;
    bool isWindows() const override;
    bool isInitialized();
    bool mkdir(const QString &path) override;

public Q_SLOTS:
    void fetchExtendedInformation(const QString &path, const QStringList &files) override;
    void updateFile(const QString &path) override;

private:
#if 0
    // called by run():
    void getFileInfos(const QString &path, const QStringList &files) override;
#endif
    QSet<QString> m_dirs; // on-going queries
    QMap<QString, QSet<QString>> m_files; // ongoing queries

    vistle::UserInterface *m_ui = nullptr;
    int m_moduleId = vistle::message::Id::Invalid;
    bool m_initialized = false;
    bool m_isWindows = false;
    QString m_hostname;
    QString m_workingDirectory;
    QString m_userName;
    QString m_homePath;
    QString m_identifier;
};

QT_END_NAMESPACE
#endif // QFILEINFOGATHERER_H
