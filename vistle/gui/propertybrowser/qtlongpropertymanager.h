#ifndef QT_LONG_PROPERTY_MANAGER
#define QT_LONG_PROPERTY_MANAGER

#include <QtAbstractPropertyManager>
#include <vistle/core/scalar.h>

class QtLongPropertyManagerPrivate;

class QT_QTPROPERTYBROWSER_EXPORT QtLongPropertyManager: public QtAbstractPropertyManager {
    Q_OBJECT
public:
    QtLongPropertyManager(QObject *parent = 0);
    ~QtLongPropertyManager();

    vistle::Integer value(const QtProperty *property) const;
    vistle::Integer minimum(const QtProperty *property) const;
    vistle::Integer maximum(const QtProperty *property) const;
    vistle::Integer singleStep(const QtProperty *property) const;
    bool isReadOnly(const QtProperty *property) const;

public Q_SLOTS:
    void setValue(QtProperty *property, vistle::Integer val);
    void setMinimum(QtProperty *property, vistle::Integer minVal);
    void setMaximum(QtProperty *property, vistle::Integer maxVal);
    void setRange(QtProperty *property, vistle::Integer minVal, vistle::Integer maxVal);
    void setSingleStep(QtProperty *property, vistle::Integer step);
    void setReadOnly(QtProperty *property, bool readOnly);
Q_SIGNALS:
    void valueChanged(QtProperty *property, vistle::Integer val);
    void rangeChanged(QtProperty *property, vistle::Integer minVal, vistle::Integer maxVal);
    void singleStepChanged(QtProperty *property, vistle::Integer step);
    void readOnlyChanged(QtProperty *property, bool readOnly);

protected:
    QString valueText(const QtProperty *property) const;
    virtual void initializeProperty(QtProperty *property);
    virtual void uninitializeProperty(QtProperty *property);

private:
    QtLongPropertyManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QtLongPropertyManager)
    Q_DISABLE_COPY(QtLongPropertyManager)
};

#endif
