#ifndef QT_LONG_PROPERTY_MANAGER
#define QT_LONG_PROPERTY_MANAGER

#include <QtAbstractPropertyManager>

class QtLongPropertyManagerPrivate;

class QT_QTPROPERTYBROWSER_EXPORT QtLongPropertyManager : public QtAbstractPropertyManager
{
    Q_OBJECT
public:
    QtLongPropertyManager(QObject *parent = 0);
    ~QtLongPropertyManager();

    long value(const QtProperty *property) const;
    long minimum(const QtProperty *property) const;
    long maximum(const QtProperty *property) const;
    long singleStep(const QtProperty *property) const;
    bool isReadOnly(const QtProperty *property) const;

public Q_SLOTS:
    void setValue(QtProperty *property, long val);
    void setMinimum(QtProperty *property, long minVal);
    void setMaximum(QtProperty *property, long maxVal);
    void setRange(QtProperty *property, long minVal, long maxVal);
    void setSingleStep(QtProperty *property, long step);
    void setReadOnly(QtProperty *property, bool readOnly);
Q_SIGNALS:
    void valueChanged(QtProperty *property, long val);
    void rangeChanged(QtProperty *property, long minVal, long maxVal);
    void singleStepChanged(QtProperty *property, long step);
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
