
#include <QObject>
#include <QDesktopServices>
#include <QUrl>
#include <QFileOpenEvent>
#include <QApplication>

#include <iostream>
#include <fstream>

#include "vistle_opener.h"

#include <vistle/util/directory.h>
#include <vistle/util/process.h>


//#define DEBUG

static std::string path_to_vistle;

static std::ostream *dbg = &std::cerr;

bool launchVistle(const std::vector<std::string> args)
{
    *dbg << "launch " << path_to_vistle << std::endl;

    std::string path;
    if (!path_to_vistle.empty()) {
        path = path_to_vistle;
    } else {
        std::string prog = "vistle";
        auto p = process::search_path(prog);
        if (p.empty()) {
            *dbg << "Cannot launch " << prog << ": not found" << std::endl;
            return false;
        }
        path = p.string();
    }

    std::vector<std::string> fullargs;
    fullargs.push_back(path);
    std::copy(args.begin(), args.end(), std::back_inserter(fullargs));

    *dbg << "launch args:";
    for (const auto &arg: fullargs) {
        *dbg << " " << arg;
    }
    *dbg << std::endl;

    try {
        process::spawn(path, process::args(args));
    } catch (std::exception &ex) {
        *dbg << "Failed to launch: " << path << ": " << ex.what() << std::endl;
        return false;
    }

    return true;
}

UrlHandler::UrlHandler(QObject *parent): QObject(parent)
{}

void UrlHandler::openUrl(const QUrl &url)
{
    *dbg << "openurl " << url.toString().toStdString() << std::endl;
    std::vector<std::string> args;
    args.push_back(url.toString().toStdString());
    launchVistle(args);
    qApp->quit();
}

FileOpenEventFilter::FileOpenEventFilter(QObject *parent): QObject(parent)
{}

bool FileOpenEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    *dbg << "event" << std::endl;

    if (event->type() == QEvent::FileOpen) {
        *dbg << "fileopen" << std::endl;
        QFileOpenEvent *fileEvent = static_cast<QFileOpenEvent *>(event);
        auto url = fileEvent->url();
        auto file = fileEvent->file();
        if (!url.isEmpty()) {
            *dbg << "url" << std::endl;
            std::vector<std::string> args{url.toString().toStdString()};
            launchVistle(args);
            qApp->quit();
        } else if (!file.isEmpty()) {
            *dbg << "file" << std::endl;
            std::vector<std::string> args{file.toStdString()};
            launchVistle(args);
            qApp->quit();
        }

        return false;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}

int main(int argc, char *argv[])
{
    std::ofstream log;
#ifdef DEBUG
    log.open("/tmp/vistle_opener.log", std::ios::app);
    if (log.is_open()) {
        dbg = &log;
    } else {
        std::cerr << "Failed to open debug log" << std::endl;
    }
#endif

    *dbg << "start" << std::endl;

    *dbg << "args: argc=" << argc << ", args:";
    for (int i = 0; i < argc; ++i) {
        *dbg << " " << argv[i];
    }
    *dbg << std::endl;

    auto dir = vistle::Directory(argc, argv);
    *dbg << "dir: prefix=" << dir.prefix() << ", bin=" << dir.bin() << ", module=" << dir.module()
         << ", moduleplugin=" << dir.moduleplugin() << ", share=" << dir.share() << ", covisedir=" << dir.covisedir()
         << std::endl;
    if (!dir.prefix().empty()) {
        *dbg << "prefix: " << dir.prefix() << std::endl;
        path_to_vistle = dir.bin() + "vistle";
        vistle::directory::setEnvironment(dir.prefix());
    }

    if (argc > 1) {
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }

        return launchVistle(args);
    }

    QApplication app(argc, argv);
    auto handler = new UrlHandler(&app);
    QDesktopServices::setUrlHandler("vistle", handler, "openUrl");
    app.installEventFilter(new FileOpenEventFilter(&app));
    app.exec();

    *dbg << "done" << std::endl;
    dbg = &std::cerr;

    return 0;
}
