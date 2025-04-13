
#include <QObject>
#include <QDesktopServices>
#include <QUrl>
#include <QFileOpenEvent>
#include <QApplication>

#include <iostream>

#include "vistle_opener.h"

#include <vistle/util/directory.h>
#include <vistle/util/process.h>


static std::string path_to_vistle;

bool launchVistle(const std::vector<std::string> args)
{
    std::cerr << "launch " << path_to_vistle << std::endl;

    std::string path;
    if (!path_to_vistle.empty()) {
        path = path_to_vistle;
    } else {
        std::string prog = "vistle";
        auto p = process::search_path(prog);
        if (p.empty()) {
            std::cerr << "Cannot launch " << prog << ": not found" << std::endl;
            return false;
        }
        path = p.string();
    }

    std::vector<std::string> fullargs;
    fullargs.push_back(path);
    std::copy(args.begin(), args.end(), std::back_inserter(fullargs));

    try {
        process::spawn(path, process::args(args));
    } catch (std::exception &ex) {
        std::cerr << "Failed to launch: " << path << ": " << ex.what() << std::endl;
        return false;
    }

    return true;
}

UrlHandler::UrlHandler(QObject *parent): QObject(parent)
{}

void UrlHandler::openUrl(const QUrl &url)
{
    std::vector<std::string> args;
    args.push_back(url.toString().toStdString());
    launchVistle(args);
    qApp->quit();
}

FileOpenEventFilter::FileOpenEventFilter(QObject *parent): QObject(parent)
{}

bool FileOpenEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    std::cerr << "event" << std::endl;

    if (event->type() == QEvent::FileOpen) {
        std::cerr << "fileopen" << std::endl;
        QFileOpenEvent *fileEvent = static_cast<QFileOpenEvent *>(event);
        auto url = fileEvent->url();
        auto file = fileEvent->file();
        if (!url.isEmpty()) {
            std::cerr << "url" << std::endl;
            std::vector<std::string> args{url.toString().toStdString()};
            launchVistle(args);
            qApp->quit();
        } else if (!file.isEmpty()) {
            std::cerr << "file" << std::endl;
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
    std::cerr << "start" << std::endl;

    auto dir = vistle::Directory(argc, argv);
    if (!dir.prefix().empty()) {
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

    std::cerr << "done" << std::endl;

    return 0;
}
