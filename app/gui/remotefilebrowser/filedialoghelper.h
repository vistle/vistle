#ifndef REMOTEFILEBROWSER_HELPER_H
#define REMOTEFILEBROWSER_HELPER_H

#include <QtCore/qobjectdefs.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qdir.h>

class RemoteFileDialogOptionsPrivate;

class RemoteFileDialogOptions {
    Q_GADGET
    Q_DISABLE_COPY(RemoteFileDialogOptions)
protected:
    RemoteFileDialogOptions(RemoteFileDialogOptionsPrivate *dd);
    ~RemoteFileDialogOptions();

public:
    enum ViewMode { Detail, List };
    Q_ENUM(ViewMode)

    enum FileMode { AnyFile, ExistingFile, Directory, ExistingFiles, DirectoryOnly };
    Q_ENUM(FileMode)

    enum AcceptMode { AcceptOpen, AcceptSave };
    Q_ENUM(AcceptMode)

    enum DialogLabel { LookIn, FileName, FileType, Accept, Reject, DialogLabelCount };
    Q_ENUM(DialogLabel)

    enum FileDialogOption {
        ShowDirsOnly = 0x00000001,
        DontResolveSymlinks = 0x00000002,
        DontConfirmOverwrite = 0x00000004,
        DontUseSheet = 0x00000008,
        DontUseNativeDialog = 0x00000010,
        ReadOnly = 0x00000020,
        HideNameFilterDetails = 0x00000040,
        DontUseCustomDirectoryIcons = 0x00000080
    };
    Q_DECLARE_FLAGS(FileDialogOptions, FileDialogOption)
    Q_FLAG(FileDialogOptions)

    static QSharedPointer<RemoteFileDialogOptions> create();
    QSharedPointer<RemoteFileDialogOptions> clone() const;

    QString windowTitle() const;
    void setWindowTitle(const QString &);

    void setOption(FileDialogOption option, bool on = true);
    bool testOption(FileDialogOption option) const;
    void setOptions(FileDialogOptions options);
    FileDialogOptions options() const;

    QDir::Filters filter() const;
    void setFilter(QDir::Filters filters);

    void setViewMode(ViewMode mode);
    ViewMode viewMode() const;

    void setFileMode(FileMode mode);
    FileMode fileMode() const;

    void setAcceptMode(AcceptMode mode);
    AcceptMode acceptMode() const;

    void setSidebarUrls(const QList<QUrl> &urls);
    QList<QUrl> sidebarUrls() const;

    bool useDefaultNameFilters() const;
    void setUseDefaultNameFilters(bool d);

    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const;

    void setMimeTypeFilters(const QStringList &filters);
    QStringList mimeTypeFilters() const;

    void setDefaultSuffix(const QString &suffix);
    QString defaultSuffix() const;

    void setHistory(const QStringList &paths);
    QStringList history() const;

    void setLabelText(DialogLabel label, const QString &text);
    QString labelText(DialogLabel label) const;
    bool isLabelExplicitlySet(DialogLabel label);

    QUrl initialDirectory() const;
    void setInitialDirectory(const QUrl &);

    QString initiallySelectedMimeTypeFilter() const;
    void setInitiallySelectedMimeTypeFilter(const QString &);

    QString initiallySelectedNameFilter() const;
    void setInitiallySelectedNameFilter(const QString &);

    QList<QUrl> initiallySelectedFiles() const;
    void setInitiallySelectedFiles(const QList<QUrl> &);

    void setSupportedSchemes(const QStringList &schemes);
    QStringList supportedSchemes() const;

    static QString defaultNameFilterString();

private:
    RemoteFileDialogOptionsPrivate *d = nullptr;
};

#if 0
class Q_GUI_EXPORT QPlatformFileDialogHelper: public QObject //: public QPlatformDialogHelper
{
    Q_OBJECT
public:
    virtual bool defaultNameFilterDisables() const = 0;
    virtual void setDirectory(const QUrl &directory) = 0;
    virtual QUrl directory() const = 0;
    virtual void selectFile(const QUrl &filename) = 0;
    virtual QList<QUrl> selectedFiles() const = 0;
    virtual void setFilter() = 0;
    virtual void selectMimeTypeFilter(const QString &filter);
    virtual void selectNameFilter(const QString &filter) = 0;
    virtual QString selectedMimeTypeFilter() const;
    virtual QString selectedNameFilter() const = 0;

    virtual bool isSupportedUrl(const QUrl &url) const;

    const QSharedPointer<RemoteFileDialogOptions> &options() const;
    void setOptions(const QSharedPointer<RemoteFileDialogOptions> &options);

    static QStringList cleanFilterList(const QString &filter);
    static const char *filterRegExp;

Q_SIGNALS:
    void fileSelected(const QUrl &file);
    void filesSelected(const QList<QUrl> &files);
    void currentChanged(const QUrl &path);
    void directoryEntered(const QUrl &directory);
    void filterSelected(const QString &filter);

private:
    QSharedPointer<RemoteFileDialogOptions> m_options;
};
#endif

#endif
