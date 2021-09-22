#ifndef QTLONGVECTORPROPERTYMANAGER_H
#define QTLONGVECTORPROPERTYMANAGER_H

#include "qtpropertybrowser.h"
#include "qtlongpropertymanager.h"
#include <QLineEdit>

#include <vistle/core/paramvector.h>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QtLongVectorPropertyManagerPrivate;

class QT_QTPROPERTYBROWSER_EXPORT QtLongVectorPropertyManager: public QtAbstractPropertyManager {
    Q_OBJECT
public:
    typedef vistle::IntParamVector IntParamVector;

    QtLongVectorPropertyManager(QObject *parent = 0);
    ~QtLongVectorPropertyManager();

    QtLongPropertyManager *subLongPropertyManager() const;

    IntParamVector value(const QtProperty *property) const;

public Q_SLOTS:
    void setValue(QtProperty *property, const IntParamVector &val);
    void setDimension(QtProperty *property, int dim);
    void setRange(QtProperty *property, const IntParamVector &min, const IntParamVector &max);
Q_SIGNALS:
    void valueChanged(QtProperty *property, IntParamVector val);
    void rangeChanged(QtProperty *property, IntParamVector min, IntParamVector max);
    void dimensionChanged(QtProperty *property, int dim);

protected:
    QString valueText(const QtProperty *property) const;
    virtual void initializeProperty(QtProperty *property);
    virtual void uninitializeProperty(QtProperty *property);

private:
    QtLongVectorPropertyManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QtLongVectorPropertyManager)
    Q_DISABLE_COPY(QtLongVectorPropertyManager)
    Q_PRIVATE_SLOT(d_func(), void slotLongChanged(QtProperty *, vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
    void valueChangedSignal(QtProperty *property, const IntParamVector &val);
    void propertyChangedSignal(QtProperty *property);
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

#endif
