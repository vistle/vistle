#include "vistlebrowserpropertymanager.h"
#include "qtpropertymanager_helpers.cpp"

#include <vistle/core/message.h>

#include <QDebug>
// VistleBrowserPropertyManager

class VistleBrowserPropertyManagerPrivate {
    VistleBrowserPropertyManager *q_ptr;
    Q_DECLARE_PUBLIC(VistleBrowserPropertyManager)
public:
    struct Data {
        Data()
        : moduleId(vistle::message::Id::Invalid)
        , filters(QString(QLatin1Char('*')))
        , regExp(".*")
        , echoMode(QLineEdit::Normal)
        , fileMode(RemoteFileDialog::AnyFile)
        , readOnly(false)
        {}
        QString val;
        int moduleId;
        QString filters;
        QRegularExpression regExp;
        int echoMode;
        int fileMode;
        bool readOnly;
        QString title;
    };

    typedef QMap<const QtProperty *, Data> PropertyValueMap;
    QMap<const QtProperty *, Data> m_values;
};

/*!
    \class VistleBrowserPropertyManager

    \brief The VistleBrowserPropertyManager provides and manages QString properties.

    A string property's value can be retrieved using the value()
    function, and set using the setValue() slot.

    The current value can be checked against a regular expression. To
    set the regular expression use the setRegExp() slot, use the
    regExp() function to retrieve the currently set expression.

    In addition, VistleBrowserPropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes, and the regExpChanged() signal which is emitted whenever
    such a property changes its currently set regular expression.

    \sa QtAbstractPropertyManager, QtLineEditFactory
*/

/*!
    \fn void VistleBrowserPropertyManager::valueChanged(QtProperty *property, const QString &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void VistleBrowserPropertyManager::regExpChanged(QtProperty *property, const QRegularExpression &regExp)

    This signal is emitted whenever a property created by this manager
    changes its currently set regular expression, passing a pointer to
    the \a property and the new \a regExp as parameters.

    \sa setRegExp()
*/

/*!
    Creates a manager with the given \a parent.
*/
VistleBrowserPropertyManager::VistleBrowserPropertyManager(QObject *parent): QtAbstractPropertyManager(parent)
{
    d_ptr = new VistleBrowserPropertyManagerPrivate;
    d_ptr->q_ptr = this;
}

/*!
    Destroys this manager, and all the properties it has created.
*/
VistleBrowserPropertyManager::~VistleBrowserPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given property is not managed by this manager, this
    function returns an empty string.

    \sa setValue()
*/
QString VistleBrowserPropertyManager::value(const QtProperty *property) const
{
    return getValue<QString>(d_ptr->m_values, property);
}

QString VistleBrowserPropertyManager::title(const QtProperty *property) const
{
    return getData<QString>(d_ptr->m_values, &VistleBrowserPropertyManagerPrivate::Data::title, property, "");
}

int VistleBrowserPropertyManager::moduleId(const QtProperty *property) const
{
    return getData<int>(d_ptr->m_values, &VistleBrowserPropertyManagerPrivate::Data::moduleId, property,
                        int(vistle::message::Id::Invalid));
}

QString VistleBrowserPropertyManager::filters(const QtProperty *property) const
{
    return getData<QString>(d_ptr->m_values, &VistleBrowserPropertyManagerPrivate::Data::filters, property,
                            QString(QLatin1Char('*')));
}

/*!
    Returns the given \a property's currently set regular expression.

    If the given \a property is not managed by this manager, this
    function returns an empty expression.

    \sa setRegExp()
*/
QRegularExpression VistleBrowserPropertyManager::regExp(const QtProperty *property) const
{
    return getData<QRegularExpression>(d_ptr->m_values, &VistleBrowserPropertyManagerPrivate::Data::regExp, property,
                                       QRegularExpression());
}

/*!
    \reimp
*/
EchoMode VistleBrowserPropertyManager::echoMode(const QtProperty *property) const
{
    return (EchoMode)getData<int>(d_ptr->m_values, &VistleBrowserPropertyManagerPrivate::Data::echoMode, property, 0);
}

/*!
    Returns read-only status of the property.

    When property is read-only it's value can be selected and copied from editor but not modified.

    \sa VistleBrowserPropertyManager::setReadOnly
*/
bool VistleBrowserPropertyManager::isReadOnly(const QtProperty *property) const
{
    return getData<bool>(d_ptr->m_values, &VistleBrowserPropertyManagerPrivate::Data::readOnly, property, false);
}

FileMode VistleBrowserPropertyManager::fileMode(const QtProperty *property) const
{
    return (FileMode)getData<int>(d_ptr->m_values, &VistleBrowserPropertyManagerPrivate::Data::fileMode, property, 0);
}

/*!
    \reimp
*/
QString VistleBrowserPropertyManager::valueText(const QtProperty *property) const
{
    const VistleBrowserPropertyManagerPrivate::PropertyValueMap::const_iterator it =
        d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();

    return it.value().val;
}

/*!
    \reimp
*/
QString VistleBrowserPropertyManager::displayText(const QtProperty *property) const
{
    const VistleBrowserPropertyManagerPrivate::PropertyValueMap::const_iterator it =
        d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();

    QLineEdit edit;
    edit.setEchoMode((EchoMode)it.value().echoMode);
    edit.setText(it.value().val);
    return edit.displayText();
}

/*!
    \fn void VistleBrowserPropertyManager::setValue(QtProperty *property, const QString &value)

    Sets the value of the given \a property to \a value.

    If the specified \a value doesn't match the given \a property's
    regular expression, this function does nothing.

    \sa value(), setRegExp(), valueChanged()
*/
void VistleBrowserPropertyManager::setValue(QtProperty *property, const QString &val)
{
    const VistleBrowserPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    VistleBrowserPropertyManagerPrivate::Data data = it.value();

    if (data.val == val)
        return;

    if (data.regExp.isValid() && !data.regExp.match(val).hasMatch())
        return;

    data.val = val;

    it.value() = data;

    emit propertyChanged(property);
    emit valueChanged(property, data.val);
}

void VistleBrowserPropertyManager::setTitle(QtProperty *property, const QString &title)
{
    const VistleBrowserPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    VistleBrowserPropertyManagerPrivate::Data data = it.value();

    if (data.title == title)
        return;

    data.title = title;

    it.value() = data;

    emit propertyChanged(property);
    emit titleChanged(property, data.title);
}

void VistleBrowserPropertyManager::setModuleId(QtProperty *property, const int id)
{
    const VistleBrowserPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    VistleBrowserPropertyManagerPrivate::Data data = it.value();

    if (data.moduleId == id)
        return;

    data.moduleId = id;

    it.value() = data;

    emit moduleIdChanged(property, data.moduleId);
}

void VistleBrowserPropertyManager::setFilters(QtProperty *property, const QString &filters)
{
    const VistleBrowserPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    VistleBrowserPropertyManagerPrivate::Data data = it.value();

    if (data.filters == filters)
        return;

    data.filters = filters;

    it.value() = data;

    emit filtersChanged(property, data.filters);
}

/*!
    Sets the regular expression of the given \a property to \a regExp.

    \sa regExp(), setValue(), regExpChanged()
*/
void VistleBrowserPropertyManager::setRegExp(QtProperty *property, const QRegularExpression &regExp)
{
    const VistleBrowserPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    VistleBrowserPropertyManagerPrivate::Data data = it.value();

    if (data.regExp == regExp)
        return;

    data.regExp = regExp;

    it.value() = data;

    emit regExpChanged(property, data.regExp);
}


void VistleBrowserPropertyManager::setEchoMode(QtProperty *property, EchoMode echoMode)
{
    const VistleBrowserPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    VistleBrowserPropertyManagerPrivate::Data data = it.value();

    if (data.echoMode == echoMode)
        return;

    data.echoMode = echoMode;
    it.value() = data;

    emit propertyChanged(property);
    emit echoModeChanged(property, data.echoMode);
}

/*!
    Sets read-only status of the property.

    \sa VistleBrowserPropertyManager::setReadOnly
*/
void VistleBrowserPropertyManager::setReadOnly(QtProperty *property, bool readOnly)
{
    const VistleBrowserPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    VistleBrowserPropertyManagerPrivate::Data data = it.value();

    if (data.readOnly == readOnly)
        return;

    data.readOnly = readOnly;
    it.value() = data;

    emit propertyChanged(property);
    emit readOnlyChanged(property, data.readOnly);
}

void VistleBrowserPropertyManager::setFileMode(QtProperty *property, FileMode fileMode)
{
    const VistleBrowserPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    VistleBrowserPropertyManagerPrivate::Data data = it.value();

    if (data.fileMode == fileMode)
        return;

    data.fileMode = fileMode;
    it.value() = data;

    emit propertyChanged(property);
    emit fileModeChanged(property, data.fileMode);
}

/*!
    \reimp
*/
void VistleBrowserPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = VistleBrowserPropertyManagerPrivate::Data();
}

/*!
    \reimp
*/
void VistleBrowserPropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}
