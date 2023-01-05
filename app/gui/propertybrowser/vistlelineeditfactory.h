#ifndef VISTLE_LINEEDIT_FACTORY
#define VISTLE_LINEEDIT_FACTORY

#include <QtLineEditFactory>

class VistleLineEditFactoryPrivate;

class QT_QTPROPERTYBROWSER_EXPORT VistleLineEditFactory: public QtAbstractEditorFactory<QtStringPropertyManager> {
    Q_OBJECT
public:
    VistleLineEditFactory(QObject *parent = 0);
    ~VistleLineEditFactory();

protected:
    void connectPropertyManager(QtStringPropertyManager *manager);
    QWidget *createEditor(QtStringPropertyManager *manager, QtProperty *property, QWidget *parent);
    void disconnectPropertyManager(QtStringPropertyManager *manager);

private:
    VistleLineEditFactoryPrivate *d_ptr;
    Q_DECLARE_PRIVATE(VistleLineEditFactory)
    Q_DISABLE_COPY(VistleLineEditFactory)
    Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QString &))
    Q_PRIVATE_SLOT(d_func(), void slotRegExpChanged(QtProperty *, const QRegularExpression &))
    Q_PRIVATE_SLOT(d_func(), void slotEchoModeChanged(QtProperty *, int))
    Q_PRIVATE_SLOT(d_func(), void slotReadOnlyChanged(QtProperty *, bool))
    Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QString &))
    Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};


#if 0
class QT_QTPROPERTYBROWSER_EXPORT VistleLineEditFactory: public QtLineEditFactory {
    Q_OBJECT

public:
    VistleLineEditFactory(QObject *parent = 0);
    QWidget *createEditor(QtStringPropertyManager *manager, QtProperty *property,
                QWidget *parent);

};
#endif

#endif
