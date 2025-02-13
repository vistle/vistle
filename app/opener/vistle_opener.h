#ifndef VISTLE_OPENER_VISTLE_OPENER_H
#define VISTLE_OPENER_VISTLE_OPENER_H

#include <QObject>

class UrlHandler: public QObject {
    Q_OBJECT
public:
    UrlHandler(QObject *parent);
public slots:
    void openUrl(const QUrl &url);
};

class FileOpenEventFilter: public QObject {
    Q_OBJECT
public:
    FileOpenEventFilter(QObject *parent);
public slots:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif
