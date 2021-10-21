#include "remotefileiconprovider.h"
#include "abstractfileinfogatherer.h"

#include <QFileIconProvider>
#include <QApplication>
#include <QStyle>

QScopedPointer<QFileIconProvider> RemoteFileIconProvider::s_qIconProvider;

RemoteFileIconProvider::RemoteFileIconProvider()
{
    if (!s_qIconProvider)
        s_qIconProvider.reset(new QFileIconProvider);
}

RemoteFileIconProvider::~RemoteFileIconProvider()
{}

QIcon RemoteFileIconProvider::icon(RemoteFileIconProvider::IconType type) const
{
    switch (type) {
    case HomeDir:
        return qApp->style()->standardIcon(QStyle::SP_DirHomeIcon);
    case Computer:
    case Desktop:
    case Trashcan:
    case Network:
    case Drive:
    case Folder:
    case File:
        return s_qIconProvider->icon(static_cast<QFileIconProvider::IconType>(type));
    }

    return s_qIconProvider->icon(QFileIconProvider::Network);
}

QIcon RemoteFileIconProvider::icon(const FileInfo &info) const
{
    if (!info.m_valid)
        return icon(Network);

    if (!info.m_exists)
        return icon(Trashcan);

    QStyle::StandardPixmap icon = QStyle::SP_FileIcon;
    switch (info.m_type) {
    case FileInfo::Unknown:
        return s_qIconProvider->icon(QFileIconProvider::Network);
    case FileInfo::Dir:
        icon = QStyle::SP_DirIcon;
        if (info.m_isSymlink)
            icon = QStyle::SP_DirLinkIcon;
        break;
    case FileInfo::File:
        icon = QStyle::SP_FileIcon;
        if (info.m_isSymlink)
            icon = QStyle::SP_FileLinkIcon;
        break;
    case FileInfo::System:
        return s_qIconProvider->icon(QString("/dev"));
    }

    return qApp->style()->standardIcon(icon);
}

QString RemoteFileIconProvider::type(const FileInfo &info) const
{
    if (!info.m_valid)
        return QString("Invalid");

    if (!info.m_exists)
        return QString("Non-existing");

    switch (info.m_type) {
    case FileInfo::Unknown:
        return QString("Unknown");
    case FileInfo::Dir:
        if (info.m_isSymlink)
            return QString("Directory (link)");
        return QString("Directory");
    case FileInfo::File:
        if (info.m_isSymlink)
            return QString("File (link)");
        return QString("File");
    case FileInfo::System:
        return QString("System");
    }

    return QString("Unknown");
}
