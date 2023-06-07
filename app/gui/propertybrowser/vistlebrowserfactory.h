#ifndef VISTLE_BROWSER_FACTORY
#define VISTLE_BROWSER_FACTORY

#include <QtLineEditFactory>
#include "vistlebrowserpropertymanager.h"

class VistleBrowserFactoryPrivate;

namespace vistle {
class UserInterface;
}

class QT_QTPROPERTYBROWSER_EXPORT VistleBrowserFactory: public QtAbstractEditorFactory<VistleBrowserPropertyManager> {
    Q_OBJECT
public:
    VistleBrowserFactory(QObject *parent = 0);
    ~VistleBrowserFactory();
    void setUi(vistle::UserInterface *ui);

protected:
    void connectPropertyManager(VistleBrowserPropertyManager *manager);
    QWidget *createEditor(VistleBrowserPropertyManager *manager, QtProperty *property, QWidget *parent);
    void disconnectPropertyManager(VistleBrowserPropertyManager *manager);

private:
    VistleBrowserFactoryPrivate *d_ptr;
    Q_DECLARE_PRIVATE(VistleBrowserFactory)
    Q_DISABLE_COPY(VistleBrowserFactory)
    Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QString &))
    Q_PRIVATE_SLOT(d_func(), void slotModuleIdChanged(QtProperty *, int))
    Q_PRIVATE_SLOT(d_func(), void slotFiltersChanged(QtProperty *, const QString &))
    Q_PRIVATE_SLOT(d_func(), void slotRegExpChanged(QtProperty *, const QRegularExpression &))
    Q_PRIVATE_SLOT(d_func(), void slotEchoModeChanged(QtProperty *, int))
    Q_PRIVATE_SLOT(d_func(), void slotReadOnlyChanged(QtProperty *, bool))
    Q_PRIVATE_SLOT(d_func(), void slotFileModeChanged(QtProperty *, int))
    Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QString &))
    Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
    Q_PRIVATE_SLOT(d_func(), void slotTitleChanged(QtProperty *, const QString &))
};
#endif
