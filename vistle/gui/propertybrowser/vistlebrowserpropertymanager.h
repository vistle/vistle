#ifndef VISTLE_BROWSERPROPERTYMANAGER_H
#define VISTLE_BROWSERPROPERTYMANAGER_H

#include <QtStringPropertyManager>

#include "vistlebrowseredit.h"

typedef VistleBrowserEdit::FileMode FileMode;

class VistleBrowserPropertyManagerPrivate;

class QT_QTPROPERTYBROWSER_EXPORT VistleBrowserPropertyManager : public QtAbstractPropertyManager
{
    Q_OBJECT
public:
    VistleBrowserPropertyManager(QObject *parent = 0);
    ~VistleBrowserPropertyManager();

    QString value(const QtProperty *property) const;
    QRegExp regExp(const QtProperty *property) const;
    EchoMode echoMode(const QtProperty *property) const;
    bool isReadOnly(const QtProperty *property) const;
    FileMode fileMode(const QtProperty *property) const;


public Q_SLOTS:
    void setValue(QtProperty *property, const QString &val);
    void setRegExp(QtProperty *property, const QRegExp &regExp);
    void setEchoMode(QtProperty *property, EchoMode echoMode);
    void setReadOnly(QtProperty *property, bool readOnly);
    void setFileMode(QtProperty *property, FileMode fileMode);

Q_SIGNALS:
    void valueChanged(QtProperty *property, const QString &val);
    void regExpChanged(QtProperty *property, const QRegExp &regExp);
    void echoModeChanged(QtProperty *property, const int);
    void readOnlyChanged(QtProperty *property, bool);
    void fileModeChanged(QtProperty *property, const int);

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
