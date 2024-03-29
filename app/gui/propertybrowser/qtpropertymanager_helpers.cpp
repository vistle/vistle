// copied from qtpropertymanager.cpp

template<class PrivateData, class Value>
static void setSimpleMinimumData(PrivateData *data, const Value &minVal)
{
    data->minVal = minVal;
    if (data->maxVal < data->minVal)
        data->maxVal = data->minVal;

    if (data->val < data->minVal)
        data->val = data->minVal;
}

template<class PrivateData, class Value>
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
template<class Value>
void orderBorders(Value &minVal, Value &maxVal)
{
    if (minVal > maxVal)
        qSwap(minVal, maxVal);
}

template<class Value>
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

//XXX: currently not needed
#if 0
void orderBorders(QSize &minVal, QSize &maxVal)
{
    orderSizeBorders(minVal, maxVal);
}

void orderBorders(QSizeF &minVal, QSizeF &maxVal)
{
    orderSizeBorders(minVal, maxVal);
}
#endif

} // namespace
} // namespace

template<class Value, class PrivateData>
static Value getData(const QMap<const QtProperty *, PrivateData> &propertyMap, Value PrivateData::*data,
                     const QtProperty *property, const Value &defaultValue = Value())
{
    typedef QMap<const QtProperty *, PrivateData> PropertyToData;
    typedef typename PropertyToData::const_iterator PropertyToDataConstIterator;
    const PropertyToDataConstIterator it = propertyMap.constFind(property);
    if (it == propertyMap.constEnd())
        return defaultValue;
    return it.value().*data;
}

template<class Value, class PrivateData>
static Value getValue(const QMap<const QtProperty *, PrivateData> &propertyMap, const QtProperty *property,
                      const Value &defaultValue = Value())
{
    return getData<Value>(propertyMap, &PrivateData::val, property, defaultValue);
}

template<class Value, class PrivateData>
static Value getMinimum(const QMap<const QtProperty *, PrivateData> &propertyMap, const QtProperty *property,
                        const Value &defaultValue = Value())
{
    return getData<Value>(propertyMap, &PrivateData::minVal, property, defaultValue);
}

template<class Value, class PrivateData>
static Value getMaximum(const QMap<const QtProperty *, PrivateData> &propertyMap, const QtProperty *property,
                        const Value &defaultValue = Value())
{
    return getData<Value>(propertyMap, &PrivateData::maxVal, property, defaultValue);
}

template<class ValueChangeParameter, class Value, class PropertyManager>
static void setSimpleValue(QMap<const QtProperty *, Value> &propertyMap, PropertyManager *manager,
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

    emit(manager->*propertyChangedSignal)(property);
    emit(manager->*valueChangedSignal)(property, val);
}

template<class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value>
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

    emit(manager->*propertyChangedSignal)(property);
    emit(manager->*valueChangedSignal)(property, data.val);
}

template<class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value>
static void
setBorderValues(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
                void (PropertyManager::*propertyChangedSignal)(QtProperty *),
                void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
                void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
                QtProperty *property, const Value &minVal, const Value &maxVal,
                void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, ValueChangeParameter,
                                                                    ValueChangeParameter, ValueChangeParameter))
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

    emit(manager->*rangeChangedSignal)(property, data.minVal, data.maxVal);

    if (setSubPropertyRange)
        (managerPrivate->*setSubPropertyRange)(property, data.minVal, data.maxVal, data.val);

    if (data.val == oldVal)
        return;

    emit(manager->*propertyChangedSignal)(property);
    emit(manager->*valueChangedSignal)(property, data.val);
}

template<class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value, class PrivateData>
static void
setBorderValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
               void (PropertyManager::*propertyChangedSignal)(QtProperty *),
               void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
               void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
               QtProperty *property, Value (PrivateData::*getRangeVal)() const,
               void (PrivateData::*setRangeVal)(ValueChangeParameter), const Value &borderVal,
               void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, ValueChangeParameter,
                                                                   ValueChangeParameter, ValueChangeParameter))
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

    emit(manager->*rangeChangedSignal)(property, data.minVal, data.maxVal);

    if (setSubPropertyRange)
        (managerPrivate->*setSubPropertyRange)(property, data.minVal, data.maxVal, data.val);

    if (data.val == oldVal)
        return;

    emit(manager->*propertyChangedSignal)(property);
    emit(manager->*valueChangedSignal)(property, data.val);
}

template<class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value, class PrivateData>
static void setMinimumValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
                            void (PropertyManager::*propertyChangedSignal)(QtProperty *),
                            void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
                            void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter,
                                                                        ValueChangeParameter),
                            QtProperty *property, const Value &minVal)
{
    void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, ValueChangeParameter, ValueChangeParameter,
                                                        ValueChangeParameter) = 0;
    setBorderValue<ValueChangeParameter, PropertyManagerPrivate, PropertyManager, Value, PrivateData>(
        manager, managerPrivate, propertyChangedSignal, valueChangedSignal, rangeChangedSignal, property,
        &PropertyManagerPrivate::Data::minimumValue, &PropertyManagerPrivate::Data::setMinimumValue, minVal,
        setSubPropertyRange);
}

template<class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value, class PrivateData>
static void setMaximumValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
                            void (PropertyManager::*propertyChangedSignal)(QtProperty *),
                            void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
                            void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter,
                                                                        ValueChangeParameter),
                            QtProperty *property, const Value &maxVal)
{
    void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, ValueChangeParameter, ValueChangeParameter,
                                                        ValueChangeParameter) = 0;
    setBorderValue<ValueChangeParameter, PropertyManagerPrivate, PropertyManager, Value, PrivateData>(
        manager, managerPrivate, propertyChangedSignal, valueChangedSignal, rangeChangedSignal, property,
        &PropertyManagerPrivate::Data::maximumValue, &PropertyManagerPrivate::Data::setMaximumValue, maxVal,
        setSubPropertyRange);
}
