#include "qtlongpropertymanager.h"

#include "qtpropertymanager_helpers.cpp"

// adapted from QtIntPropertyManager (qtpropertymanager.cpp)
//
class QtLongPropertyManagerPrivate {
    QtLongPropertyManager *q_ptr;
    Q_DECLARE_PUBLIC(QtLongPropertyManager)
public:
    struct Data {
        Data(): val(0), minVal(LONG_MIN), maxVal(LONG_MAX), singleStep(1), readOnly(false) {}
        vistle::Integer val;
        vistle::Integer minVal;
        vistle::Integer maxVal;
        vistle::Integer singleStep;
        bool readOnly;
        vistle::Integer minimumValue() const { return minVal; }
        vistle::Integer maximumValue() const { return maxVal; }
        void setMinimumValue(vistle::Integer newMinVal) { setSimpleMinimumData(this, newMinVal); }
        void setMaximumValue(vistle::Integer newMaxVal) { setSimpleMaximumData(this, newMaxVal); }
    };

    typedef QMap<const QtProperty *, Data> PropertyValueMap;
    PropertyValueMap m_values;
};


/*!
    \class QtLongPropertyManager

    \brief The QtLongPropertyManager provides and manages vistle::Integer properties.

    An vistle::Integer property has a current value, and a range specifying the
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
    \fn void QtLongPropertyManager::valueChanged(QtProperty *property, vistle::Integer value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtLongPropertyManager::rangeChanged(QtProperty *property, vistle::Integer minimum, vistle::Integer maximum)

    This signal is emitted whenever a property created by this manager
    changes its range of valid values, passing a pointer to the
    \a property and the new \a minimum and \a maximum values.

    \sa setRange()
*/

/*!
    \fn void QtLongPropertyManager::singleStepChanged(QtProperty *property, vistle::Integer step)

    This signal is emitted whenever a property created by this manager
    changes its single step property, passing a pointer to the
    \a property and the new \a step value

    \sa setSingleStep()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtLongPropertyManager::QtLongPropertyManager(QObject *parent): QtAbstractPropertyManager(parent)
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
vistle::Integer QtLongPropertyManager::value(const QtProperty *property) const
{
    return getValue<vistle::Integer>(d_ptr->m_values, property, 0);
}

/*!
    Returns the given \a property's minimum value.

    \sa setMinimum(), maximum(), setRange()
*/
vistle::Integer QtLongPropertyManager::minimum(const QtProperty *property) const
{
    return getMinimum<vistle::Integer>(d_ptr->m_values, property, 0);
}

/*!
    Returns the given \a property's maximum value.

    \sa setMaximum(), minimum(), setRange()
*/
vistle::Integer QtLongPropertyManager::maximum(const QtProperty *property) const
{
    return getMaximum<vistle::Integer>(d_ptr->m_values, property, 0);
}

/*!
    Returns the given \a property's step value.

    The step is typically used to increment or decrement a property value while pressing an arrow key.

    \sa setSingleStep()
*/
vistle::Integer QtLongPropertyManager::singleStep(const QtProperty *property) const
{
    return getData<vistle::Integer>(d_ptr->m_values, &QtLongPropertyManagerPrivate::Data::singleStep, property, 0);
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
    \fn void QtLongPropertyManager::setValue(QtProperty *property, vistle::Integer value)

    Sets the value of the given \a property to \a value.

    If the specified \a value is not valid according to the given \a
    property's range, the \a value is adjusted to the nearest valid
    value within the range.

    \sa value(), setRange(), valueChanged()
*/
void QtLongPropertyManager::setValue(QtProperty *property, vistle::Integer val)
{
    void (QtLongPropertyManagerPrivate::*setSubPropertyValue)(QtProperty *, vistle::Integer) = 0;
    setValueInRange<vistle::Integer, QtLongPropertyManagerPrivate, QtLongPropertyManager, vistle::Integer>(
        this, d_ptr, &QtLongPropertyManager::propertyChanged, &QtLongPropertyManager::valueChanged, property, val,
        setSubPropertyValue);
}

/*!
    Sets the minimum value for the given \a property to \a minVal.

    When setting the minimum value, the maximum and current values are
    adjusted if necessary (ensuring that the range remains valid and
    that the current value is within the range).

    \sa minimum(), setRange(), rangeChanged()
*/
void QtLongPropertyManager::setMinimum(QtProperty *property, vistle::Integer minVal)
{
    setMinimumValue<vistle::Integer, QtLongPropertyManagerPrivate, QtLongPropertyManager, vistle::Integer,
                    QtLongPropertyManagerPrivate::Data>(this, d_ptr, &QtLongPropertyManager::propertyChanged,
                                                        &QtLongPropertyManager::valueChanged,
                                                        &QtLongPropertyManager::rangeChanged, property, minVal);
}

/*!
    Sets the maximum value for the given \a property to \a maxVal.

    When setting maximum value, the minimum and current values are
    adjusted if necessary (ensuring that the range remains valid and
    that the current value is within the range).

    \sa maximum(), setRange(), rangeChanged()
*/
void QtLongPropertyManager::setMaximum(QtProperty *property, vistle::Integer maxVal)
{
    setMaximumValue<vistle::Integer, QtLongPropertyManagerPrivate, QtLongPropertyManager, vistle::Integer,
                    QtLongPropertyManagerPrivate::Data>(this, d_ptr, &QtLongPropertyManager::propertyChanged,
                                                        &QtLongPropertyManager::valueChanged,
                                                        &QtLongPropertyManager::rangeChanged, property, maxVal);
}

/*!
    \fn void QtLongPropertyManager::setRange(QtProperty *property, vistle::Integer minimum, vistle::Integer maximum)

    Sets the range of valid values.

    This is a convenience function defining the range of valid values
    in one go; setting the \a minimum and \a maximum values for the
    given \a property with a single function call.

    When setting a new range, the current value is adjusted if
    necessary (ensuring that the value remains within range).

    \sa setMinimum(), setMaximum(), rangeChanged()
*/
void QtLongPropertyManager::setRange(QtProperty *property, vistle::Integer minVal, vistle::Integer maxVal)
{
    void (QtLongPropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, vistle::Integer, vistle::Integer,
                                                              vistle::Integer) = 0;
    setBorderValues<vistle::Integer, QtLongPropertyManagerPrivate, QtLongPropertyManager, vistle::Integer>(
        this, d_ptr, &QtLongPropertyManager::propertyChanged, &QtLongPropertyManager::valueChanged,
        &QtLongPropertyManager::rangeChanged, property, minVal, maxVal, setSubPropertyRange);
}

/*!
    Sets the step value for the given \a property to \a step.

    The step is typically used to increment or decrement a property value while pressing an arrow key.

    \sa singleStep()
*/
void QtLongPropertyManager::setSingleStep(QtProperty *property, vistle::Integer step)
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
