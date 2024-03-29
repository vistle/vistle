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

#ifndef QSIDEBAR_H
#define QSIDEBAR_H

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

#include <qlistview.h>
#include <qstandarditemmodel.h>
#include <qstyleditemdelegate.h>
#include <qurl.h>
#include <qvector.h>

QT_REQUIRE_CONFIG(filedialog);

QT_BEGIN_NAMESPACE

class AbstractFileSystemModel;

class RFBSideBarDelegate: public QStyledItemDelegate {
public:
    RFBSideBarDelegate(QWidget *parent = 0): QStyledItemDelegate(parent) {}
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

class Q_AUTOTEST_EXPORT RemoteUrlModel: public QStandardItemModel {
    Q_OBJECT
    friend class RFBSidebar;

public:
    enum Roles { UrlRole = Qt::UserRole + 1, EnabledRole = Qt::UserRole + 2 };

    RemoteUrlModel(QObject *parent = 0);

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
#ifndef QT_NO_DRAGANDDROP
    bool canDrop(QDragEnterEvent *event);
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                      const QModelIndex &parent) override;
#endif
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void setUrls(const QList<QUrl> &list);
    void addUrls(const QList<QUrl> &urls, int row = -1, bool move = true);
    QList<QUrl> urls() const;
    void setFileSystemModel(AbstractFileSystemModel *model);
    bool showFullPath;

public Q_SLOTS:
    void updateHomePath();

private Q_SLOTS:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void layoutChanged();

private:
    void setUrl(const QModelIndex &index, const QUrl &url, const QModelIndex &dirIndex);
    void changed(const QString &path);
    void addIndexToWatch(const QString &path, const QModelIndex &index);
    AbstractFileSystemModel *fileSystemModel;
    struct WatchItem {
        QModelIndex index;
        QString path;
    };
    friend class QTypeInfo<WatchItem>;

    QVector<WatchItem> watching;
    QList<QUrl> invalidUrls;
};
Q_DECLARE_TYPEINFO(RemoteUrlModel::WatchItem, Q_MOVABLE_TYPE);

class Q_AUTOTEST_EXPORT RFBSidebar: public QListView {
    Q_OBJECT

Q_SIGNALS:
    void goToUrl(const QUrl &url);

public:
    RFBSidebar(QWidget *parent = 0);
    void setModelAndUrls(AbstractFileSystemModel *model, const QList<QUrl> &newUrls);
    ~RFBSidebar();

    QSize sizeHint() const override;

    void setUrls(const QList<QUrl> &list) { urlModel->setUrls(list); }
    void addUrls(const QList<QUrl> &list, int row) { urlModel->addUrls(list, row); }
    QList<QUrl> urls() const { return urlModel->urls(); }

    void selectUrl(const QUrl &url);

protected:
    bool event(QEvent *e) override;
    void focusInEvent(QFocusEvent *event) override;
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *event) override;
#endif

private Q_SLOTS:
    void clicked(const QModelIndex &index);
#if QT_CONFIG(menu)
    void showContextMenu(const QPoint &position);
#endif
    void removeEntry();

private:
    RemoteUrlModel *urlModel;
};

QT_END_NAMESPACE

#endif // QSIDEBAR_H
