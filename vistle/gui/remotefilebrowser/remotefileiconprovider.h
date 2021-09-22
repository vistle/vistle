#ifndef REMOTE_FILEICON_PROVIDER_H
#define REMOTE_FILEICON_PROVIDER_H

#include <QFileIconProvider>

class FileInfo;

class RemoteFileIconProvider {
public:
    RemoteFileIconProvider();
    virtual ~RemoteFileIconProvider();
    enum IconType {
        Computer = QFileIconProvider::Computer,
        Desktop = QFileIconProvider::Desktop,
        Trashcan = QFileIconProvider::Trashcan,
        Network = QFileIconProvider::Network,
        Drive = QFileIconProvider::Drive,
        Folder = QFileIconProvider::Folder,
        File = QFileIconProvider::File,
        HomeDir
    };

    virtual QIcon icon(IconType type) const;
    virtual QIcon icon(const FileInfo &info) const;
    virtual QString type(const FileInfo &info) const;

private:
    static QScopedPointer<QFileIconProvider> s_qIconProvider;
};
#endif
