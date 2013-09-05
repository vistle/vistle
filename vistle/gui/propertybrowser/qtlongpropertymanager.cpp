#include "qtlongpropertymanager.h"

// copied from qtpropertymanager.cpp

template <class PrivateData, class Value>
static void setSimpleMinimumData(PrivateData *data, const Value &minVal)
{
    data->minVal = minVal;
    if (data->maxVal < data->minVal)
        data->maxVal = data->minVal;

    if (data->val < data->minVal)
        data->val = data->minVal;
}

template <class PrivateData, class Value>
static void setSimpleMaximumData(PrivateData *data, const Value &maxVal)
{
    data->maxVal = maxVal;
    if (data->minVal > data->maxVal)
        data->minVal = data->maxVal;

    if (data->val > data->maxVal)
        data->val = data->maxVal;
}


namespace {

namespace {
template <class Value>
void orderBorders(Value &minVal, Value &maxVal)
{
    if (minVal > maxVal)
        qSwap(minVal, maxVal);
}

template <class Value>
static void orderSizeBorders(Value &minVal, Value &maxVal)
{
    Value fromSize = minVal;
    Value toSize = maxVal;
    if (fromSize.width() > toSize.width()) {
        fromSize.setWidth(maxVal.width());
        toSize.setWidth(minVal.width());
    }
    if (fromSize.height() > toSize.height()) {
        fromSize.setHeight(maxVal.height());
        toSize.setHeight(minVal.height());
    }
    minVal = fromSize;
    maxVal = toSize;
}

void orderBorders(QSize &minVal, QSize &maxVal)
{
    orderSizeBorders(minVal, maxVal);
}

void orderBorders(QSizeF &minVal, QSizeF &maxVal)
{
    orderSizeBorders(minVal, maxVal);
}

}
}

template <class Value, class PrivateData>
static Value getData(const QMap<const QtProperty *, PrivateData> &propertyMap,
            Value PrivateData::*data,
            const QtProperty *property, const Value &defaultValue = Value())
{
    typedef QMap<const QtProperty *, PrivateData> PropertyToData;
    typedef typename PropertyToData::const_iterator PropertyToDataConstIterator;
    const PropertyToDataConstIterator it = propertyMap.constFind(property);
    if (it == propertyMap.constEnd())
        return defaultValue;
    return it.value().*data;
}

template <class Value, class PrivateData>
static Value getValue(const QMap<const QtProperty *, PrivateData> &propertyMap,
            const QtProperty *property, const Value &defaultValue = Value())
{
    return getData<Value>(propertyMap, &PrivateData::val, property, defaultValue);
}

template <class Value, class PrivateData>
static Value getMinimum(const QMap<const QtProperty *, PrivateData> &propertyMap,
            const QtProperty *property, const Value &defaultValue = Value())
{
    return getData<Value>(propertyMap, &PrivateData::minVal, property, defaultValue);
}

template <class Value, class PrivateData>
static Value getMaximum(const QMap<const QtProperty *, PrivateData> &propertyMap,
            const QtProperty *property, const Value &defaultValue = Value())
{
    return getData<Value>(propertyMap, &PrivateData::maxVal, property, defaultValue);
}

template <class ValueChangeParameter, class Value, class PropertyManager>
static void setSimpleValue(QMap<const QtProperty *, Value> &propertyMap,
            PropertyManager *manager,
            void (PropertyManager::*propertyChangedSignal)(QtProperty *),
            void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
            QtProperty *property, const Value &val)
{
    typedef QMap<const QtProperty *, Value> PropertyToData;
    typedef typename PropertyToData::iterator PropertyToDataIterator;
    const PropertyToDataIterator it = propertyMap.find(property);
    if (it == propertyMap.end())
        return;

    if (it.value() == val)
        return;

    it.value() = val;

    emit (manager->*propertyChangedSignal)(property);
    emit (manager->*valueChangedSignal)(property, val);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value>
static void setValueInRange(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
            void (PropertyManager::*propertyChangedSignal)(QtProperty *),
            void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
            QtProperty *property, const Value &val,
            void (PropertyManagerPrivate::*setSubPropertyValue)(QtProperty *, ValueChangeParameter))
{
    typedef typename PropertyManagerPrivate::Data PrivateData;
    typedef QMap<const QtProperty *, PrivateData> PropertyToData;
    typedef typename PropertyToData::iterator PropertyToDataIterator;
    const PropertyToDataIterator it = managerPrivate->m_values.find(property);
    if (it == managerPrivate->m_values.end())
        return;

    PrivateData &data = it.value();

    if (data.val == val)
        return;

    const Value oldVal = data.val;

    data.val = qBound(data.minVal, val, data.maxVal);

    if (data.val == oldVal)
        return;

    if (setSubPropertyValue)
        (managerPrivate->*setSubPropertyValue)(property, data.val);

    emit (manager->*propertyChangedSignal)(property);
    emit (manager->*valueChangedSignal)(property, data.val);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value>
static void setBorderValues(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
            void (PropertyManager::*propertyChangedSignal)(QtProperty *),
            void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
            void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
            QtProperty *property, const Value &minVal, const Value &maxVal,
            void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *,
                    ValueChangeParameter, ValueChangeParameter, ValueChangeParameter))
{
    typedef typename PropertyManagerPrivate::Data PrivateData;
    typedef QMap<const QtProperty *, PrivateData> PropertyToData;
    typedef typename PropertyToData::iterator PropertyToDataIterator;
    const PropertyToDataIterator it = managerPrivate->m_values.find(property);
    if (it == managerPrivate->m_values.end())
        return;

    Value fromVal = minVal;
    Value toVal = maxVal;
    orderBorders(fromVal, toVal);

    PrivateData &data = it.value();

    if (data.minVal == fromVal && data.maxVal == toVal)
        return;

    const Value oldVal = data.val;

    data.setMinimumValue(fromVal);
    data.setMaximumValue(toVal);

    emit (manager->*rangeChangedSignal)(property, data.minVal, data.maxVal);

    if (setSubPropertyRange)
        (managerPrivate->*setSubPropertyRange)(property, data.minVal, data.maxVal, data.val);

    if (data.val == oldVal)
        return;

    emit (manager->*propertyChangedSignal)(property);
    emit (manager->*valueChangedSignal)(property, data.val);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value, class PrivateData>
static void setBorderValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
            void (PropertyManager::*propertyChangedSignal)(QtProperty *),
            void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
            void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
            QtProperty *property,
            Value (PrivateData::*getRangeVal)() const,
            void (PrivateData::*setRangeVal)(ValueChangeParameter), const Value &borderVal,
            void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *,
                    ValueChangeParameter, ValueChangeParameter, ValueChangeParameter))
{
    typedef QMap<const QtProperty *, PrivateData> PropertyToData;
    typedef typename PropertyToData::iterator PropertyToDataIterator;
    const PropertyToDataIterator it = managerPrivate->m_values.find(property);
    if (it == managerPrivate->m_values.end())
        return;

    PrivateData &data = it.value();

    if ((data.*getRangeVal)() == borderVal)
        return;

    const Value oldVal = data.val;

    (data.*setRangeVal)(borderVal);

    emit (manager->*rangeChangedSignal)(property, data.minVal, data.maxVal);

    if (setSubPropertyRange)
        (managerPrivate->*setSubPropertyRange)(property, data.minVal, data.maxVal, data.val);

    if (data.val == oldVal)
        return;

    emit (manager->*propertyChangedSignal)(property);
    emit (manager->*valueChangedSignal)(property, data.val);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value, class PrivateData>
static void setMinimumValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
            void (PropertyManager::*propertyChangedSignal)(QtProperty *),
            void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
            void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
            QtProperty *property, const Value &minVal)
{
    void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *,
                    ValueChangeParameter, ValueChangeParameter, ValueChangeParameter) = 0;
    setBorderValue<ValueChangeParameter, PropertyManagerPrivate, PropertyManager, Value, PrivateData>(manager, managerPrivate,
            propertyChangedSignal, valueChangedSignal, rangeChangedSignal,
            property, &PropertyManagerPrivate::Data::minimumValue, &PropertyManagerPrivate::Data::setMinimumValue, minVal, setSubPropertyRange);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value, class PrivateData>
static void setMaximumValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
            void (PropertyManager::*propertyChangedSignal)(QtProperty *),
            void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
            void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
            QtProperty *property, const Value &maxVal)
{
    void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *,
                    ValueChangeParameter, ValueChangeParameter, ValueChangeParameter) = 0;
    setBorderValue<ValueChangeParameter, PropertyManagerPrivate, PropertyManager, Value, PrivateData>(manager, managerPrivate,
            propertyChangedSignal, valueChangedSignal, rangeChangedSignal,
            property, &PropertyManagerPrivate::Data::maximumValue, &PropertyManagerPrivate::Data::setMaximumValue, maxVal, setSubPropertyRange);
}


// adapted from QtIntPropertyManager (qtpropertymanager.cpp)
//
class QtLongPropertyManagerPrivate
{
    QtLongPropertyManager *q_ptr;
    Q_DECLARE_PUBLIC(QtLongPropertyManager)
public:

    struct Data
    {
        Data() : val(0), minVal(-LONG_MAX), maxVal(LONG_MAX), singleStep(1), readOnly(false) {}
        long val;
        long minVal;
        long maxVal;
        long singleStep;
        bool readOnly;
        long minimumValue() const { return minVal; }
        long maximumValue() const { return maxVal; }
        void setMinimumValue(long newMinVal) { setSimpleMinimumData(this, newMinVal); }
        void setMaximumValue(long newMaxVal) { setSimpleMaximumData(this, newMaxVal); }
    };

    typedef QMap<const QtProperty *, Data> PropertyValueMap;
    PropertyValueMap m_values;
};


/*!
    \class QtLongPropertyManager

    \brief The QtLongPropertyManager provides and manages long properties.

    An long property has a current value, and a range specifying the
    valid values. The range is defined by a minimum and a maximum
    value.

    The property's value and range can be retrieved using the value(),
    minimum() and maximum() functions, and can be set using the
    setValue(), setMinimum() and setMaximum() slots. Alternatively,
    the range can be defined in one go using the setRange() slot.

    In addition, QtLongPropertyManager provides the valueChanged() signal which
    is emitted whenever a property created by this manager changes,
    and the rangeChanged() signal which is emitted whenever such a
    property changes its range of valid values.

    \sa QtAbstractPropertyManager, QtSpinBoxFactory, QtSliderFactory, QtScrollBarFactory
*/

/*!
    \fn void QtLongPropertyManager::valueChanged(QtProperty *property, long value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtLongPropertyManager::rangeChanged(QtProperty *property, long minimum, long maximum)

    This signal is emitted whenever a property created by this manager
    changes its range of valid values, passing a pointer to the
    \a property and the new \a minimum and \a maximum values.

    \sa setRange()
*/

/*!
    \fn void QtLongPropertyManager::singleStepChanged(QtProperty *property, long step)

    This signal is emitted whenever a property created by this manager
    changes its single step property, passing a pointer to the
    \a property and the new \a step value

    \sa setSingleStep()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtLongPropertyManager::QtLongPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtLongPropertyManagerPrivate;
    d_ptr->q_ptr = this;
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtLongPropertyManager::~QtLongPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given property is not managed by this manager, this
    function returns 0.

    \sa setValue()
*/
long QtLongPropertyManager::value(const QtProperty *property) const
{
    return getValue<long>(d_ptr->m_values, property, 0);
}

/*!
    Returns the given \a property's minimum value.

    \sa setMinimum(), maximum(), setRange()
*/
long QtLongPropertyManager::minimum(const QtProperty *property) const
{
    return getMinimum<long>(d_ptr->m_values, property, 0);
}

/*!
    Returns the given \a property's maximum value.

    \sa setMaximum(), minimum(), setRange()
*/
long QtLongPropertyManager::maximum(const QtProperty *property) const
{
    return getMaximum<long>(d_ptr->m_values, property, 0);
}

/*!
    Returns the given \a property's step value.

    The step is typically used to increment or decrement a property value while pressing an arrow key.

    \sa setSingleStep()
*/
long QtLongPropertyManager::singleStep(const QtProperty *property) const
{
    return getData<long>(d_ptr->m_values, &QtLongPropertyManagerPrivate::Data::singleStep, property, 0);
}

/*!
    Returns read-only status of the property.

    When property is read-only it's value can be selected and copied from editor but not modified.

    \sa QtLongPropertyManager::setReadOnly
*/
bool QtLongPropertyManager::isReadOnly(const QtProperty *property) const
{
    return getData<bool>(d_ptr->m_values, &QtLongPropertyManagerPrivate::Data::readOnly, property, false);
}

/*!
    \reimp
*/
QString QtLongPropertyManager::valueText(const QtProperty *property) const
{
    const QtLongPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    return QString::number(it.value().val);
}

/*!
    \fn void QtLongPropertyManager::setValue(QtProperty *property, long value)

    Sets the value of the given \a property to \a value.

    If the specified \a value is not valid according to the given \a
    property's range, the \a value is adjusted to the nearest valid
    value within the range.

    \sa value(), setRange(), valueChanged()
*/
void QtLongPropertyManager::setValue(QtProperty *property, long val)
{
    void (QtLongPropertyManagerPrivate::*setSubPropertyValue)(QtProperty *, long) = 0;
    setValueInRange<long, QtLongPropertyManagerPrivate, QtLongPropertyManager, long>(this, d_ptr,
                &QtLongPropertyManager::propertyChanged,
                &QtLongPropertyManager::valueChanged,
                property, val, setSubPropertyValue);
}

/*!
    Sets the minimum value for the given \a property to \a minVal.

    When setting the minimum value, the maximum and current values are
    adjusted if necessary (ensuring that the range remains valid and
    that the current value is within the range).

    \sa minimum(), setRange(), rangeChanged()
*/
void QtLongPropertyManager::setMinimum(QtProperty *property, long minVal)
{
    setMinimumValue<long, QtLongPropertyManagerPrivate, QtLongPropertyManager, long, QtLongPropertyManagerPrivate::Data>(this, d_ptr,
                &QtLongPropertyManager::propertyChanged,
                &QtLongPropertyManager::valueChanged,
                &QtLongPropertyManager::rangeChanged,
                property, minVal);
}

/*!
    Sets the maximum value for the given \a property to \a maxVal.

    When setting maximum value, the minimum and current values are
    adjusted if necessary (ensuring that the range remains valid and
    that the current value is within the range).

    \sa maximum(), setRange(), rangeChanged()
*/
void QtLongPropertyManager::setMaximum(QtProperty *property, long maxVal)
{
    setMaximumValue<long, QtLongPropertyManagerPrivate, QtLongPropertyManager, long, QtLongPropertyManagerPrivate::Data>(this, d_ptr,
                &QtLongPropertyManager::propertyChanged,
                &QtLongPropertyManager::valueChanged,
                &QtLongPropertyManager::rangeChanged,
                property, maxVal);
}

/*!
    \fn void QtLongPropertyManager::setRange(QtProperty *property, long minimum, long maximum)

    Sets the range of valid values.

    This is a convenience function defining the range of valid values
    in one go; setting the \a minimum and \a maximum values for the
    given \a property with a single function call.

    When setting a new range, the current value is adjusted if
    necessary (ensuring that the value remains within range).

    \sa setMinimum(), setMaximum(), rangeChanged()
*/
void QtLongPropertyManager::setRange(QtProperty *property, long minVal, long maxVal)
{
    void (QtLongPropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, long, long, long) = 0;
    setBorderValues<long, QtLongPropertyManagerPrivate, QtLongPropertyManager, long>(this, d_ptr,
                &QtLongPropertyManager::propertyChanged,
                &QtLongPropertyManager::valueChanged,
                &QtLongPropertyManager::rangeChanged,
                property, minVal, maxVal, setSubPropertyRange);
}

/*!
    Sets the step value for the given \a property to \a step.

    The step is typically used to increment or decrement a property value while pressing an arrow key.

    \sa singleStep()
*/
void QtLongPropertyManager::setSingleStep(QtProperty *property, long step)
{
    const QtLongPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtLongPropertyManagerPrivate::Data data = it.value();

    if (step < 0)
        step = 0;

    if (data.singleStep == step)
        return;

    data.singleStep = step;

    it.value() = data;

    emit singleStepChanged(property, data.singleStep);
}

/*!
    Sets read-only status of the property.

    \sa QtLongPropertyManager::setReadOnly
*/
void QtLongPropertyManager::setReadOnly(QtProperty *property, bool readOnly)
{
    const QtLongPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtLongPropertyManagerPrivate::Data data = it.value();

    if (data.readOnly == readOnly)
        return;

    data.readOnly = readOnly;
    it.value() = data;

    emit propertyChanged(property);
    emit readOnlyChanged(property, data.readOnly);
}

/*!
    \reimp
*/
void QtLongPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtLongPropertyManagerPrivate::Data();
}

/*!
    \reimp
*/
void QtLongPropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}
