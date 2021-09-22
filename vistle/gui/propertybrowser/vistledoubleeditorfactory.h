#ifndef VISTLE_DOUBLE_SPINBOX_FACTORY
#define VISTLE_DOUBLE_SPINBOX_FACTORY

#include <QtDoubleSpinBoxFactory>

class VistleDoubleSpinBoxFactoryPrivate;

class QT_QTPROPERTYBROWSER_EXPORT VistleDoubleSpinBoxFactory: public QtAbstractEditorFactory<QtDoublePropertyManager> {
    Q_OBJECT
public:
    VistleDoubleSpinBoxFactory(QObject *parent = 0);
    ~VistleDoubleSpinBoxFactory();

protected:
    void connectPropertyManager(QtDoublePropertyManager *manager);
    QWidget *createEditor(QtDoublePropertyManager *manager, QtProperty *property, QWidget *parent);
    void disconnectPropertyManager(QtDoublePropertyManager *manager);

private:
    VistleDoubleSpinBoxFactoryPrivate *d_ptr;
    Q_DECLARE_PRIVATE(VistleDoubleSpinBoxFactory)
    Q_DISABLE_COPY(VistleDoubleSpinBoxFactory)
    Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, double))
    Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, double, double))
    Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(QtProperty *, double))
    Q_PRIVATE_SLOT(d_func(), void slotDecimalsChanged(QtProperty *, int))
    Q_PRIVATE_SLOT(d_func(), void slotReadOnlyChanged(QtProperty *, bool))
    Q_PRIVATE_SLOT(d_func(), void slotSetValue(double))
    Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

#endif
