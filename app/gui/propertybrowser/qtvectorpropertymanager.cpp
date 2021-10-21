#include "qtvectorpropertymanager.h"
#include "vistledoublepropertymanager.h"

#include "qtpropertymanager_helpers.cpp"

using vistle::ParamVector;

// QtVectorPropertyManager

class QtVectorPropertyManagerPrivate {
    QtVectorPropertyManager *q_ptr;
    Q_DECLARE_PUBLIC(QtVectorPropertyManager)
public:
    struct Data {
        Data(): decimals(2), dimension(0) {}
        void setMinimumValue(const ParamVector &val) { minVal = val; }
        void setMaximumValue(const ParamVector &val) { maxVal = val; }
        ParamVector val;
        ParamVector minVal;
        ParamVector maxVal;
        int decimals;
        int dimension;
    };

    void slotDoubleChanged(QtProperty *property, double value);
    void slotPropertyDestroyed(QtProperty *property);

    typedef QMap<const QtProperty *, Data> PropertyValueMap;
    PropertyValueMap m_values;

    VistleDoublePropertyManager *m_doublePropertyManager;

    QMap<const QtProperty *, QtProperty *> m_toSub[vistle::MaxDimension];
    QMap<const QtProperty *, QtProperty *> m_fromSub[vistle::MaxDimension];
};

void QtVectorPropertyManagerPrivate::slotDoubleChanged(QtProperty *property, double value)
{
    for (int i = 0; i < vistle::MaxDimension; ++i) {
        if (QtProperty *prop = m_fromSub[i].value(property, nullptr)) {
            ParamVector &p = m_values[prop].val;
            p[i] = value;
            q_ptr->propertyChangedSignal(prop);
            q_ptr->valueChangedSignal(prop, p);
            break;
        }
    }
}

void QtVectorPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    for (int i = 0; i < vistle::MaxDimension; ++i) {
        if (QtProperty *prop = m_fromSub[i].value(property, nullptr)) {
            m_toSub[i][prop] = nullptr;
            m_fromSub[i].remove(property);
            break;
        }
    }
}

/*! \class QtVectorPropertyManager

    \brief The QtVectorPropertyManager provides and manages ParamVector properties.

    A point property has nested \e x and \e y subproperties. The
    top-level property's value can be retrieved using the value()
    function, and set using the setValue() slot.

    The subproperties are created by a QtDoublePropertyManager object. This
    manager can be retrieved using the subDoublePropertyManager() function. In
    order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    In addition, QtVectorPropertyManager provides the valueChanged() signal which
    is emitted whenever a property created by this manager changes.

    \sa QtAbstractPropertyManager, QtDoublePropertyManager, QtPointPropertyManager
*/

/*!
    \fn void QtVectorPropertyManager::valueChanged(QtProperty *property, const ParamVector &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtVectorPropertyManager::decimalsChanged(QtProperty *property, int prec)

    This signal is emitted whenever a property created by this manager
    changes its precision of value, passing a pointer to the
    \a property and the new \a prec value

    \sa setDecimals()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtVectorPropertyManager::QtVectorPropertyManager(QObject *parent): QtAbstractPropertyManager(parent)
{
    d_ptr = new QtVectorPropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_doublePropertyManager = new VistleDoublePropertyManager(this);
    connect(d_ptr->m_doublePropertyManager, SIGNAL(valueChanged(QtProperty *, double)), this,
            SLOT(slotDoubleChanged(QtProperty *, double)));
    connect(d_ptr->m_doublePropertyManager, SIGNAL(propertyDestroyed(QtProperty *)), this,
            SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtVectorPropertyManager::~QtVectorPropertyManager()
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
QtDoublePropertyManager *QtVectorPropertyManager::subDoublePropertyManager() const
{
    return d_ptr->m_doublePropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns a point with coordinates (0, 0).

    \sa setValue()
*/
ParamVector QtVectorPropertyManager::value(const QtProperty *property) const
{
    return getValue<ParamVector>(d_ptr->m_values, property);
}

/*!
    Returns the given \a property's precision, in decimals.

    \sa setDecimals()
*/
int QtVectorPropertyManager::decimals(const QtProperty *property) const
{
    return getData<int>(d_ptr->m_values, &QtVectorPropertyManagerPrivate::Data::decimals, property, 0);
}

/*!
    \reimp
*/
QString QtVectorPropertyManager::valueText(const QtProperty *property) const
{
    const QtVectorPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    const ParamVector v = it.value().val;
    //const int dec =  it.value().decimals;

    if (v.dim == 0)
        return "()";

    QString res;
    for (int i = 0; i < v.dim; ++i) {
        if (i > 0)
            res += ", ";
        res += QString::number(v[i], 'g', 2 /*dec*/);
    }
    res = "(" + res + ")";
    return res;
}

/*!
    \fn void QtVectorPropertyManager::setValue(QtProperty *property, const ParamVector &value)

    Sets the value of the given \a property to \a value. Nested
    properties are updated automatically.

    \sa value(), valueChanged()
*/
void QtVectorPropertyManager::setValue(QtProperty *property, const ParamVector &val)
{
    const QtVectorPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    if (it.value().val == val)
        return;

    if (it.value().dimension != val.dim) {
        setDimension(property, val.dim);
    }

    it.value().val = val;
    for (int i = 0; i < val.dim; ++i) {
        d_ptr->m_doublePropertyManager->setValue(d_ptr->m_toSub[i][property], val[i]);
    }

    emit propertyChanged(property);
    emit valueChanged(property, val);
}

/*!
    \fn void QtVectorPropertyManager::setDecimals(QtProperty *property, int prec)

    Sets the precision of the given \a property to \a prec.

    The valid decimal range is 0-13. The default is 2.

    \sa decimals()
*/
void QtVectorPropertyManager::setDecimals(QtProperty *property, int prec)
{
    const QtVectorPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtVectorPropertyManagerPrivate::Data data = it.value();

    if (prec > 13)
        prec = 13;
    else if (prec < 0)
        prec = 0;

    if (data.decimals == prec)
        return;

    data.decimals = prec;
    for (int i = 0; i < vistle::MaxDimension; ++i) {
        d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_toSub[i][property], prec);
    }

    it.value() = data;

    emit decimalsChanged(property, data.decimals);
}

void QtVectorPropertyManager::setDimension(QtProperty *property, int dim)
{
    const QtVectorPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtVectorPropertyManagerPrivate::Data data = it.value();

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

void QtVectorPropertyManager::setRange(QtProperty *property, const ParamVector &minVal, const ParamVector &maxVal)
{
    void (QtVectorPropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, ParamVector, ParamVector, ParamVector) =
        0;
    setBorderValues<ParamVector, QtVectorPropertyManagerPrivate, QtVectorPropertyManager, ParamVector>(
        this, d_ptr, &QtVectorPropertyManager::propertyChanged, &QtVectorPropertyManager::valueChanged,
        &QtVectorPropertyManager::rangeChanged, property, minVal, maxVal, setSubPropertyRange);
}

/*!
    \reimp
*/
void QtVectorPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtVectorPropertyManagerPrivate::Data();

    for (int i = 0; i < vistle::MaxDimension; ++i) {
        QtProperty *sub = d_ptr->m_doublePropertyManager->addProperty();

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

        d_ptr->m_doublePropertyManager->setDecimals(sub, decimals(property));
        d_ptr->m_doublePropertyManager->setValue(sub, 0.);
        d_ptr->m_toSub[i][property] = sub;
        d_ptr->m_fromSub[i][sub] = property;

        //property->addSubProperty(sub);
    }
}

/*!
    \reimp
*/
void QtVectorPropertyManager::uninitializeProperty(QtProperty *property)
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

void QtVectorPropertyManager::valueChangedSignal(QtProperty *property, const vistle::ParamVector &val)
{
    emit valueChanged(property, val);
}

void QtVectorPropertyManager::propertyChangedSignal(QtProperty *property)
{
    emit propertyChanged(property);
}

#include "moc_qtvectorpropertymanager.cpp"
