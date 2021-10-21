#include "qtlongvectorpropertymanager.h"

#include "qtpropertymanager_helpers.cpp"

using vistle::IntParamVector;

// QtLongVectorPropertyManager

class QtLongVectorPropertyManagerPrivate {
    QtLongVectorPropertyManager *q_ptr;
    Q_DECLARE_PUBLIC(QtLongVectorPropertyManager)
public:
    struct Data {
        Data(): dimension(0) {}
        void setMinimumValue(const IntParamVector &val) { minVal = val; }
        void setMaximumValue(const IntParamVector &val) { maxVal = val; }
        IntParamVector val;
        IntParamVector minVal;
        IntParamVector maxVal;
        int dimension;
    };

    void slotLongChanged(QtProperty *property, vistle::Integer value);
    void slotPropertyDestroyed(QtProperty *property);

    typedef QMap<const QtProperty *, Data> PropertyValueMap;
    PropertyValueMap m_values;

    QtLongPropertyManager *m_longPropertyManager;

    QMap<const QtProperty *, QtProperty *> m_toSub[vistle::MaxDimension];
    QMap<const QtProperty *, QtProperty *> m_fromSub[vistle::MaxDimension];
};

void QtLongVectorPropertyManagerPrivate::slotLongChanged(QtProperty *property, vistle::Integer value)
{
    for (int i = 0; i < vistle::MaxDimension; ++i) {
        if (QtProperty *prop = m_fromSub[i].value(property, nullptr)) {
            IntParamVector &p = m_values[prop].val;
            p[i] = value;
            q_ptr->propertyChangedSignal(prop);
            q_ptr->valueChangedSignal(prop, p);
            break;
        }
    }
}

void QtLongVectorPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    for (int i = 0; i < vistle::MaxDimension; ++i) {
        if (QtProperty *prop = m_fromSub[i].value(property, nullptr)) {
            m_toSub[i][prop] = nullptr;
            m_fromSub[i].remove(property);
            break;
        }
    }
}

/*! \class QtLongVectorPropertyManager

    \brief The QtLongVectorPropertyManager provides and manages IntParamVector properties.

    A point property has nested \e x and \e y subproperties. The
    top-level property's value can be retrieved using the value()
    function, and set using the setValue() slot.

    The subproperties are created by a QtLongPropertyManager object. This
    manager can be retrieved using the subLongPropertyManager() function. In
    order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    In addition, QtLongVectorPropertyManager provides the valueChanged() signal which
    is emitted whenever a property created by this manager changes.

    \sa QtAbstractPropertyManager, QtLongPropertyManager, QtPointPropertyManager
*/

/*!
    \fn void QtLongVectorPropertyManager::valueChanged(QtProperty *property, const IntParamVector &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.

    \sa setValue()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtLongVectorPropertyManager::QtLongVectorPropertyManager(QObject *parent): QtAbstractPropertyManager(parent)
{
    d_ptr = new QtLongVectorPropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_longPropertyManager = new QtLongPropertyManager(this);
    connect(d_ptr->m_longPropertyManager, SIGNAL(valueChanged(QtProperty *, vistle::Integer)), this,
            SLOT(slotLongChanged(QtProperty *, vistle::Integer)));
    connect(d_ptr->m_longPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)), this,
            SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtLongVectorPropertyManager::~QtLongVectorPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e x and \e y
    subproperties.

    In order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtLongPropertyManager *QtLongVectorPropertyManager::subLongPropertyManager() const
{
    return d_ptr->m_longPropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns a point with coordinates (0, 0).

    \sa setValue()
*/
IntParamVector QtLongVectorPropertyManager::value(const QtProperty *property) const
{
    return getValue<IntParamVector>(d_ptr->m_values, property);
}

/*!
    \reimp
*/
QString QtLongVectorPropertyManager::valueText(const QtProperty *property) const
{
    const QtLongVectorPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    const IntParamVector v = it.value().val;

    if (v.dim == 0)
        return "()";

    QString res;
    for (int i = 0; i < v.dim; ++i) {
        if (i > 0)
            res += ", ";
        res += QString::number(v[i]);
    }
    res = "(" + res + ")";
    return res;
}

/*!
    \fn void QtLongVectorPropertyManager::setValue(QtProperty *property, const IntParamVector &value)

    Sets the value of the given \a property to \a value. Nested
    properties are updated automatically.

    \sa value(), valueChanged()
*/
void QtLongVectorPropertyManager::setValue(QtProperty *property, const IntParamVector &val)
{
    const QtLongVectorPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    if (it.value().val == val)
        return;

    if (it.value().dimension != val.dim) {
        setDimension(property, val.dim);
    }

    it.value().val = val;
    for (int i = 0; i < val.dim; ++i) {
        d_ptr->m_longPropertyManager->setValue(d_ptr->m_toSub[i][property], val[i]);
    }

    emit propertyChanged(property);
    emit valueChanged(property, val);
}

void QtLongVectorPropertyManager::setDimension(QtProperty *property, int dim)
{
    const QtLongVectorPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtLongVectorPropertyManagerPrivate::Data data = it.value();

    if (dim > vistle::MaxDimension)
        dim = vistle::MaxDimension;

    if (data.dimension == dim)
        return;

    data.dimension = dim;
    for (int i = 0; i < dim; ++i) {
        property->addSubProperty(d_ptr->m_toSub[i][property]);
    }

    emit dimensionChanged(property, data.dimension);
}

void QtLongVectorPropertyManager::setRange(QtProperty *property, const IntParamVector &minVal,
                                           const IntParamVector &maxVal)
{
    void (QtLongVectorPropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, IntParamVector, IntParamVector,
                                                                    IntParamVector) = 0;
    setBorderValues<IntParamVector, QtLongVectorPropertyManagerPrivate, QtLongVectorPropertyManager, IntParamVector>(
        this, d_ptr, &QtLongVectorPropertyManager::propertyChanged, &QtLongVectorPropertyManager::valueChanged,
        &QtLongVectorPropertyManager::rangeChanged, property, minVal, maxVal, setSubPropertyRange);
}

/*!
    \reimp
*/
void QtLongVectorPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtLongVectorPropertyManagerPrivate::Data();

    for (int i = 0; i < vistle::MaxDimension; ++i) {
        QtProperty *sub = d_ptr->m_longPropertyManager->addProperty();

        QString n = QString::number(i);
        switch (i) {
        case 0:
            n = "x";
            break;
        case 1:
            n = "y";
            break;
        case 2:
            n = "z";
            break;
        case 3:
            n = "w";
            break;
        default:
            break;
        }
        sub->setPropertyName(n);

        d_ptr->m_longPropertyManager->setValue(sub, 0.);
        d_ptr->m_toSub[i][property] = sub;
        d_ptr->m_fromSub[i][sub] = property;

        //property->addSubProperty(sub);
    }
}

/*!
    \reimp
*/
void QtLongVectorPropertyManager::uninitializeProperty(QtProperty *property)
{
    for (int i = 0; i < vistle::MaxDimension; ++i) {
        QtProperty *sub = d_ptr->m_toSub[i][property];
        if (sub) {
            d_ptr->m_fromSub[i].remove(sub);
            delete sub;
        }
        d_ptr->m_toSub[i].remove(property);
    }
}

void QtLongVectorPropertyManager::valueChangedSignal(QtProperty *property, const vistle::IntParamVector &val)
{
    emit valueChanged(property, val);
}

void QtLongVectorPropertyManager::propertyChangedSignal(QtProperty *property)
{
    emit propertyChanged(property);
}

#include "moc_qtlongvectorpropertymanager.cpp"
