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

#ifndef REMOTEFILESYSTEMMODEL_H
#define REMOTEFILESYSTEMMODEL_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qpair.h>
#include <QtCore/qdir.h>
#include <QtGui/qicon.h>
#include <QtCore/qdiriterator.h>

#include "abstractfileinfogatherer.h"

QT_REQUIRE_CONFIG(filesystemmodel);

QT_BEGIN_NAMESPACE

class ExtendedInformation;
class RemoteFileSystemModelPrivate;
class AbstractFileInfoGatherer;
class RemoteFileIconProvider;

#include "abstractfilesystemmodel.h"

class RemoteFileSystemModel: public AbstractFileSystemModel {
    Q_OBJECT
    Q_PROPERTY(bool resolveSymlinks READ resolveSymlinks WRITE setResolveSymlinks)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool nameFilterDisables READ nameFilterDisables WRITE setNameFilterDisables)

Q_SIGNALS:
    void initialized();
    void rootPathChanged(const QString &newPath);
    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void directoryLoaded(const QString &path);

public:
    RemoteFileSystemModel(AbstractFileInfoGatherer *fileinfogatherer, QObject *parent = nullptr);
    ~RemoteFileSystemModel() override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(const QString &path, int column = 0) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    using QObject::parent;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QString identifier() const override;
    QVariant myComputer(int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    Qt::DropActions supportedDropActions() const override;

    bool isRootDir(const QString &path) const override;
    QVariant homePath(int role = Qt::DisplayRole) const override;
    QString workingDirectory() const override;
    bool isWindows() const override;
    QString userName() const override;

    // RemoteFileSystemModel specific API
    QModelIndex setRootPath(const QString &path) override;
    QString rootPath() const override;

    void setIconProvider(RemoteFileIconProvider *provider) override;
    RemoteFileIconProvider *iconProvider() const override;

    void setDisableRecursiveSort(bool enable) override;
    bool disableRecursiveSort() const override;

    void setFilter(QDir::Filters filters) override;
    QDir::Filters filter() const override;

    void setResolveSymlinks(bool enable) override;
    bool resolveSymlinks() const override;

    void setReadOnly(bool enable) override;
    bool isReadOnly() const override;

    void setNameFilterDisables(bool enable) override;
    bool nameFilterDisables() const override;

    void setNameFilters(const QStringList &filters) override;
    QStringList nameFilters() const override;

    QString filePath(const QModelIndex &index) const override;
    bool isDir(const QModelIndex &index) const override;
    qint64 size(const QModelIndex &index) const override;
    QString type(const QModelIndex &index) const override;
    QDateTime lastModified(const QModelIndex &index) const override;

    QModelIndex mkdir(const QModelIndex &parent, const QString &name) override;
    inline QString fileName(const QModelIndex &index) const override;
    inline QIcon fileIcon(const QModelIndex &index) const override;
    QFile::Permissions permissions(const QModelIndex &index) const override;
    FileInfo fileInfo(const QModelIndex &index) const override;

protected:
    RemoteFileSystemModel(RemoteFileSystemModelPrivate &, QObject *parent = nullptr);
    void timerEvent(QTimerEvent *event) override;
    bool event(QEvent *event) override;
    bool indexValid(const QModelIndex &index) const;

private:
    Q_DECLARE_PRIVATE_D(vd_ptr, RemoteFileSystemModel)
    Q_DISABLE_COPY(RemoteFileSystemModel)

    Q_PRIVATE_SLOT(d_func(), void _q_modelInitialized())
    Q_PRIVATE_SLOT(d_func(), void _q_directoryChanged(const QString &directory, const QStringList &list))
    Q_PRIVATE_SLOT(d_func(), void _q_performDelayedSort())
    Q_PRIVATE_SLOT(d_func(), void _q_fileSystemChanged(const QString &path, const QVector<QPair<QString, FileInfo>> &))
    Q_PRIVATE_SLOT(d_func(), void _q_resolvedName(const QString &fileName, const QString &resolvedName))

    QScopedPointer<RemoteFileSystemModelPrivate> vd_ptr;
};

inline QString RemoteFileSystemModel::fileName(const QModelIndex &aindex) const
{
    return aindex.data(Qt::DisplayRole).toString();
}
inline QIcon RemoteFileSystemModel::fileIcon(const QModelIndex &aindex) const
{
    return qvariant_cast<QIcon>(aindex.data(Qt::DecorationRole));
}

QT_END_NAMESPACE

#endif // REMOTEFILESYSTEMMODEL_H
