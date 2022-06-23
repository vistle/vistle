#include <boost/process.hpp>

#include <QObject>
#include <QDesktopServices>
#include <QUrl>
#include <QFileOpenEvent>
#include <QApplication>

#include <iostream>

#include "vistle_opener.h"


namespace process = boost::process;

bool launchVistle(const std::vector<std::string> args)
{
    std::cerr << "launch" << std::endl;

    std::string prog = "vistle";
    auto path = process::search_path(prog);
    if (path.empty()) {
        std::cerr << "Cannot launch " << prog << ": not found" << std::endl;
        return false;
    }

    std::vector<std::string> fullargs;
    fullargs.push_back(path.string());
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
