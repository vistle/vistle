#ifndef VISTLE_BROWSERPROPERTYMANAGER_H
#define VISTLE_BROWSERPROPERTYMANAGER_H

#include <QtStringPropertyManager>
#include <QRegularExpression>

#include "vistlebrowseredit.h"

typedef VistleBrowserEdit::FileMode FileMode;

class VistleBrowserPropertyManagerPrivate;

class QT_QTPROPERTYBROWSER_EXPORT VistleBrowserPropertyManager: public QtAbstractPropertyManager {
    Q_OBJECT
public:
    VistleBrowserPropertyManager(QObject *parent = 0);
    ~VistleBrowserPropertyManager();

    QString value(const QtProperty *property) const;
    int moduleId(const QtProperty *property) const;
    QString filters(const QtProperty *property) const;
    QRegularExpression regExp(const QtProperty *property) const;
    EchoMode echoMode(const QtProperty *property) const;
    bool isReadOnly(const QtProperty *property) const;
    FileMode fileMode(const QtProperty *property) const;
    QString title(const QtProperty *property) const;


public Q_SLOTS:
    void setValue(QtProperty *property, const QString &val);
    void setModuleId(QtProperty *property, int moduleId);
    void setFilters(QtProperty *property, const QString &filters);
    void setRegExp(QtProperty *property, const QRegularExpression &regExp);
    void setEchoMode(QtProperty *property, EchoMode echoMode);
    void setReadOnly(QtProperty *property, bool readOnly);
    void setFileMode(QtProperty *property, FileMode fileMode);
    void setTitle(QtProperty *property, const QString &title);

Q_SIGNALS:
    void valueChanged(QtProperty *property, const QString &val);
    void moduleIdChanged(QtProperty *property, const int moduleId);
    void filtersChanged(QtProperty *property, const QString &filters);
    void regExpChanged(QtProperty *property, const QRegularExpression &regExp);
    void echoModeChanged(QtProperty *property, const int);
    void readOnlyChanged(QtProperty *property, bool);
    void fileModeChanged(QtProperty *property, const int);
    void titleChanged(QtProperty *property, const QString &title);

protected:
    QString valueText(const QtProperty *property) const;
    QString displayText(const QtProperty *property) const;
    virtual void initializeProperty(QtProperty *property);
    virtual void uninitializeProperty(QtProperty *property);

private:
    VistleBrowserPropertyManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(VistleBrowserPropertyManager)
    Q_DISABLE_COPY(VistleBrowserPropertyManager)
};


#endif
