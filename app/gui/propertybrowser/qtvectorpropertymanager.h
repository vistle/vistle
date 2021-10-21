#ifndef QTVECTORPROPERTYMANAGER_H
#define QTVECTORPROPERTYMANAGER_H

#include "qtpropertybrowser.h"
#include <QtDoublePropertyManager>
#include <QLineEdit>

#include <vistle/core/paramvector.h>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QtVectorPropertyManagerPrivate;

class QT_QTPROPERTYBROWSER_EXPORT QtVectorPropertyManager: public QtAbstractPropertyManager {
    Q_OBJECT
public:
    typedef vistle::ParamVector ParamVector;

    QtVectorPropertyManager(QObject *parent = 0);
    ~QtVectorPropertyManager();

    QtDoublePropertyManager *subDoublePropertyManager() const;

    ParamVector value(const QtProperty *property) const;
    int decimals(const QtProperty *property) const;

public Q_SLOTS:
    void setValue(QtProperty *property, const ParamVector &val);
    void setDecimals(QtProperty *property, int prec);
    void setDimension(QtProperty *property, int dim);
    void setRange(QtProperty *property, const ParamVector &min, const ParamVector &max);
Q_SIGNALS:
    void valueChanged(QtProperty *property, ParamVector val);
    void rangeChanged(QtProperty *property, ParamVector min, ParamVector max);
    void decimalsChanged(QtProperty *property, int prec);
    void dimensionChanged(QtProperty *property, int dim);

protected:
    QString valueText(const QtProperty *property) const;
    virtual void initializeProperty(QtProperty *property);
    virtual void uninitializeProperty(QtProperty *property);

private:
    QtVectorPropertyManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QtVectorPropertyManager)
    Q_DISABLE_COPY(QtVectorPropertyManager)
    Q_PRIVATE_SLOT(d_func(), void slotDoubleChanged(QtProperty *, double))
    Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
    void valueChangedSignal(QtProperty *property, const ParamVector &val);
    void propertyChangedSignal(QtProperty *property);
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

#endif
