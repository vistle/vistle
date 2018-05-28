#ifndef VISTLE_BROWSER_FACTORY
#define VISTLE_BROWSER_FACTORY

#include <QtLineEditFactory>
#include "vistlebrowserpropertymanager.h"

class VistleBrowserFactoryPrivate;

class QT_QTPROPERTYBROWSER_EXPORT VistleBrowserFactory : public QtAbstractEditorFactory<VistleBrowserPropertyManager>
{
    Q_OBJECT
public:
    VistleBrowserFactory(QObject *parent = 0);
    ~VistleBrowserFactory();
protected:
    void connectPropertyManager(VistleBrowserPropertyManager *manager);
    QWidget *createEditor(VistleBrowserPropertyManager *manager, QtProperty *property,
                QWidget *parent);
    void disconnectPropertyManager(VistleBrowserPropertyManager *manager);
private:
    VistleBrowserFactoryPrivate *d_ptr;
    Q_DECLARE_PRIVATE(VistleBrowserFactory)
    Q_DISABLE_COPY(VistleBrowserFactory)
    Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QString &))
    Q_PRIVATE_SLOT(d_func(), void slotRegExpChanged(QtProperty *, const QRegExp &))
    Q_PRIVATE_SLOT(d_func(), void slotEchoModeChanged(QtProperty *, int))
    Q_PRIVATE_SLOT(d_func(), void slotReadOnlyChanged(QtProperty *, bool))
    Q_PRIVATE_SLOT(d_func(), void slotFileModeChanged(QtProperty *, int))
    Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QString &))
    Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};


#if 0
class QT_QTPROPERTYBROWSER_EXPORT VistleBrowserFactory: public QtLineEditFactory {
    Q_OBJECT

public:
    VistleBrowserFactory(QObject *parent = 0);
    QWidget *createEditor(VistleBrowserPropertyManager *manager, QtProperty *property,
                QWidget *parent);

};
#endif

#endif
