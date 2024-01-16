#include <qvariant.h>
#include "remotefiledialog.h"
#include "filedialoghelper.h"
#include <QCoreApplication>

// File dialog

class RemoteFileDialogOptionsPrivate: public QSharedData {
public:
    RemoteFileDialogOptionsPrivate()
    : viewMode(RemoteFileDialogOptions::Detail)
    , fileMode(RemoteFileDialogOptions::AnyFile)
    , acceptMode(RemoteFileDialogOptions::AcceptOpen)
    , filters(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs)
    , useDefaultNameFilters(true)
    {}

    RemoteFileDialogOptions::FileDialogOptions options;
    QString windowTitle;

    RemoteFileDialogOptions::ViewMode viewMode;
    RemoteFileDialogOptions::FileMode fileMode;
    RemoteFileDialogOptions::AcceptMode acceptMode;
    QString labels[RemoteFileDialogOptions::DialogLabelCount];
    QDir::Filters filters;
    QList<QUrl> sidebarUrls;
    bool useDefaultNameFilters;
    QStringList nameFilters;
    QStringList mimeTypeFilters;
    QString defaultSuffix;
    QStringList history;
    QUrl initialDirectory;
    QString initiallySelectedMimeTypeFilter;
    QString initiallySelectedNameFilter;
    QList<QUrl> initiallySelectedFiles;
    QStringList supportedSchemes;
};

RemoteFileDialogOptions::RemoteFileDialogOptions(RemoteFileDialogOptionsPrivate *dd): d(dd)
{}

RemoteFileDialogOptions::~RemoteFileDialogOptions()
{}

namespace {
struct FileDialogCombined: RemoteFileDialogOptionsPrivate, RemoteFileDialogOptions {
    FileDialogCombined(): RemoteFileDialogOptionsPrivate(), RemoteFileDialogOptions(this) {}
    FileDialogCombined(const FileDialogCombined &other)
    : RemoteFileDialogOptionsPrivate(other), RemoteFileDialogOptions(this)
    {}
};
} // namespace

// static
QSharedPointer<RemoteFileDialogOptions> RemoteFileDialogOptions::create()
{
    return QSharedPointer<FileDialogCombined>::create();
}

QSharedPointer<RemoteFileDialogOptions> RemoteFileDialogOptions::clone() const
{
    return QSharedPointer<FileDialogCombined>::create(*static_cast<const FileDialogCombined *>(this));
}

QString RemoteFileDialogOptions::windowTitle() const
{
    return d->windowTitle;
}

void RemoteFileDialogOptions::setWindowTitle(const QString &title)
{
    d->windowTitle = title;
}

void RemoteFileDialogOptions::setOption(RemoteFileDialogOptions::FileDialogOption option, bool on)
{
    if (!(d->options & option) != !on)
        setOptions(d->options ^ option);
}

bool RemoteFileDialogOptions::testOption(RemoteFileDialogOptions::FileDialogOption option) const
{
    return d->options & option;
}

void RemoteFileDialogOptions::setOptions(FileDialogOptions options)
{
    if (options != d->options)
        d->options = options;
}

RemoteFileDialogOptions::FileDialogOptions RemoteFileDialogOptions::options() const
{
    return d->options;
}

QDir::Filters RemoteFileDialogOptions::filter() const
{
    return d->filters;
}

void RemoteFileDialogOptions::setFilter(QDir::Filters filters)
{
    d->filters = filters;
}

void RemoteFileDialogOptions::setViewMode(RemoteFileDialogOptions::ViewMode mode)
{
    d->viewMode = mode;
}

RemoteFileDialogOptions::ViewMode RemoteFileDialogOptions::viewMode() const
{
    return d->viewMode;
}

void RemoteFileDialogOptions::setFileMode(RemoteFileDialogOptions::FileMode mode)
{
    d->fileMode = mode;
}

RemoteFileDialogOptions::FileMode RemoteFileDialogOptions::fileMode() const
{
    return d->fileMode;
}

void RemoteFileDialogOptions::setAcceptMode(RemoteFileDialogOptions::AcceptMode mode)
{
    d->acceptMode = mode;
}

RemoteFileDialogOptions::AcceptMode RemoteFileDialogOptions::acceptMode() const
{
    return d->acceptMode;
}

void RemoteFileDialogOptions::setSidebarUrls(const QList<QUrl> &urls)
{
    d->sidebarUrls = urls;
}

QList<QUrl> RemoteFileDialogOptions::sidebarUrls() const
{
    return d->sidebarUrls;
}

/*!
    \since 5.7
    \internal
    The bool property useDefaultNameFilters indicates that no name filters have been
    set or that they are equivalent to \gui{All Files (*)}. If it is true, the
    platform can choose to hide the filter combo box.

    \sa defaultNameFilterString().
*/
bool RemoteFileDialogOptions::useDefaultNameFilters() const
{
    return d->useDefaultNameFilters;
}

void RemoteFileDialogOptions::setUseDefaultNameFilters(bool dnf)
{
    d->useDefaultNameFilters = dnf;
}

void RemoteFileDialogOptions::setNameFilters(const QStringList &filters)
{
    d->useDefaultNameFilters =
        filters.size() == 1 && filters.first() == RemoteFileDialogOptions::defaultNameFilterString();
    d->nameFilters = filters;
}

QStringList RemoteFileDialogOptions::nameFilters() const
{
    return d->useDefaultNameFilters ? QStringList(RemoteFileDialogOptions::defaultNameFilterString()) : d->nameFilters;
}

/*!
    \since 5.6
    \internal
    \return The translated default name filter string (\gui{All Files (*)}).
    \sa defaultNameFilters(), nameFilters()
*/

QString RemoteFileDialogOptions::defaultNameFilterString()
{
    return QCoreApplication::translate("RemoteFileDialog", "All Files (*)");
}

void RemoteFileDialogOptions::setMimeTypeFilters(const QStringList &filters)
{
    d->mimeTypeFilters = filters;
}

QStringList RemoteFileDialogOptions::mimeTypeFilters() const
{
    return d->mimeTypeFilters;
}

void RemoteFileDialogOptions::setDefaultSuffix(const QString &suffix)
{
    d->defaultSuffix = suffix;
    if (d->defaultSuffix.size() > 1 && d->defaultSuffix.startsWith(QLatin1Char('.')))
        d->defaultSuffix.remove(0, 1); // Silently change ".txt" -> "txt".
}

QString RemoteFileDialogOptions::defaultSuffix() const
{
    return d->defaultSuffix;
}

void RemoteFileDialogOptions::setHistory(const QStringList &paths)
{
    d->history = paths;
}

QStringList RemoteFileDialogOptions::history() const
{
    return d->history;
}

void RemoteFileDialogOptions::setLabelText(RemoteFileDialogOptions::DialogLabel label, const QString &text)
{
    if (label >= 0 && label < DialogLabelCount)
        d->labels[label] = text;
}

QString RemoteFileDialogOptions::labelText(RemoteFileDialogOptions::DialogLabel label) const
{
    return (label >= 0 && label < DialogLabelCount) ? d->labels[label] : QString();
}

bool RemoteFileDialogOptions::isLabelExplicitlySet(DialogLabel label)
{
    return label >= 0 && label < DialogLabelCount && !d->labels[label].isEmpty();
}

QUrl RemoteFileDialogOptions::initialDirectory() const
{
    return d->initialDirectory;
}

void RemoteFileDialogOptions::setInitialDirectory(const QUrl &directory)
{
    d->initialDirectory = directory;
}

QString RemoteFileDialogOptions::initiallySelectedMimeTypeFilter() const
{
    return d->initiallySelectedMimeTypeFilter;
}

void RemoteFileDialogOptions::setInitiallySelectedMimeTypeFilter(const QString &filter)
{
    d->initiallySelectedMimeTypeFilter = filter;
}

QString RemoteFileDialogOptions::initiallySelectedNameFilter() const
{
    return d->initiallySelectedNameFilter;
}

void RemoteFileDialogOptions::setInitiallySelectedNameFilter(const QString &filter)
{
    d->initiallySelectedNameFilter = filter;
}

QList<QUrl> RemoteFileDialogOptions::initiallySelectedFiles() const
{
    return d->initiallySelectedFiles;
}

void RemoteFileDialogOptions::setInitiallySelectedFiles(const QList<QUrl> &files)
{
    d->initiallySelectedFiles = files;
}

// Schemes supported by the application
void RemoteFileDialogOptions::setSupportedSchemes(const QStringList &schemes)
{
    d->supportedSchemes = schemes;
}

QStringList RemoteFileDialogOptions::supportedSchemes() const
{
    return d->supportedSchemes;
}

#if 0
void QPlatformFileDialogHelper::selectMimeTypeFilter(const QString &filter)
{
    Q_UNUSED(filter)
}

QString QPlatformFileDialogHelper::selectedMimeTypeFilter() const
{
    return QString();
}

// Return true if the URL is supported by the filedialog implementation *and* by the application.
bool QPlatformFileDialogHelper::isSupportedUrl(const QUrl &url) const
{
    return url.isLocalFile();
}

/*!
    \class QPlatformFileDialogHelper
    \since 5.0
    \internal
    \ingroup qpa

    \brief The QPlatformFileDialogHelper class allows for platform-specific customization of file dialogs.

*/
const QSharedPointer<RemoteFileDialogOptions> &QPlatformFileDialogHelper::options() const
{
    return m_options;
}

void QPlatformFileDialogHelper::setOptions(const QSharedPointer<RemoteFileDialogOptions> &options)
{
    m_options = options;
}

const char *QPlatformFileDialogHelper::filterRegExp =
"^(.*)\\(([a-zA-Z0-9_.,*? +;#\\-\\[\\]@\\{\\}/!<>\\$%&=^~:\\|]*)\\)$";

// Makes a list of filters from a normal filter string "Image Files (*.png *.jpg)"
QStringList QPlatformFileDialogHelper::cleanFilterList(const QString &filter)
{
    QRegularExpression regexp(QString::fromLatin1(filterRegExp));
    Q_ASSERT(regexp.isValid());
    QString f = filter;
    int i = regexp.indexIn(f);
    if (i >= 0)
        f = regexp.cap(2);
    return f.split(QLatin1Char(' '), QString::SkipEmptyParts);
}
#endif

#include "moc_filedialoghelper.cpp"
