#include "vistledoubleeditorfactory.h"

#include <QDoubleSpinBox>

#include "qteditorfactory.h"
#include "qtpropertybrowserutils_p.h"
#include "propertyeditorfactory_p.h"
#include <cfloat>
#include <cmath>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class VistleDoubleSpinBox: public QDoubleSpinBox {
public:
    VistleDoubleSpinBox(QWidget *parent = nullptr): QDoubleSpinBox(parent) { setDecimals(DBL_MAX_10_EXP + DBL_DIG); }

    QString textFromValue(double value) const { return QString::number(value, 'g', 15); }

    QValidator::State validate(QString &text, int &pos) const { return QValidator::Acceptable; }
};

// VistleDoubleSpinBoxFactory

class VistleDoubleSpinBoxFactoryPrivate: public EditorFactoryPrivate<VistleDoubleSpinBox> {
    VistleDoubleSpinBoxFactory *q_ptr;
    Q_DECLARE_PUBLIC(VistleDoubleSpinBoxFactory)
public:
    void slotPropertyChanged(QtProperty *property, double value);
    void slotRangeChanged(QtProperty *property, double min, double max);
    void slotSingleStepChanged(QtProperty *property, double step);
    void slotDecimalsChanged(QtProperty *property, int prec);
    void slotReadOnlyChanged(QtProperty *property, bool readOnly);
    void slotSetValue(double value);
};

void VistleDoubleSpinBoxFactoryPrivate::slotPropertyChanged(QtProperty *property, double value)
{
    QList<VistleDoubleSpinBox *> editors = m_createdEditors[property];
    QListIterator<VistleDoubleSpinBox *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        VistleDoubleSpinBox *editor = itEditor.next();
        if (editor->value() != value) {
            editor->blockSignals(true);
            editor->setValue(value);
            editor->blockSignals(false);
        }
    }
}

void VistleDoubleSpinBoxFactoryPrivate::slotRangeChanged(QtProperty *property, double min, double max)
{
    if (!m_createdEditors.contains(property))
        return;

    QtDoublePropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QList<VistleDoubleSpinBox *> editors = m_createdEditors[property];
    QListIterator<VistleDoubleSpinBox *> itEditor(editors);
    while (itEditor.hasNext()) {
        VistleDoubleSpinBox *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setRange(min, max);
        editor->setValue(manager->value(property));
        editor->blockSignals(false);
    }
}

void VistleDoubleSpinBoxFactoryPrivate::slotSingleStepChanged(QtProperty *property, double step)
{
    if (!m_createdEditors.contains(property))
        return;

    QtDoublePropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QList<VistleDoubleSpinBox *> editors = m_createdEditors[property];
    QListIterator<VistleDoubleSpinBox *> itEditor(editors);
    while (itEditor.hasNext()) {
        VistleDoubleSpinBox *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setSingleStep(step);
        editor->blockSignals(false);
    }
}

void VistleDoubleSpinBoxFactoryPrivate::slotReadOnlyChanged(QtProperty *property, bool readOnly)
{
    if (!m_createdEditors.contains(property))
        return;

    QtDoublePropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<VistleDoubleSpinBox *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        VistleDoubleSpinBox *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setReadOnly(readOnly);
        editor->blockSignals(false);
    }
}

void VistleDoubleSpinBoxFactoryPrivate::slotDecimalsChanged(QtProperty *property, int prec)
{
    if (!m_createdEditors.contains(property))
        return;

    QtDoublePropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QList<VistleDoubleSpinBox *> editors = m_createdEditors[property];
    QListIterator<VistleDoubleSpinBox *> itEditor(editors);
    while (itEditor.hasNext()) {
        VistleDoubleSpinBox *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setDecimals(prec);
        editor->setValue(manager->value(property));
        editor->blockSignals(false);
    }
}

void VistleDoubleSpinBoxFactoryPrivate::slotSetValue(double value)
{
    QObject *object = q_ptr->sender();
    const QMap<VistleDoubleSpinBox *, QtProperty *>::ConstIterator itcend = m_editorToProperty.constEnd();
    for (QMap<VistleDoubleSpinBox *, QtProperty *>::ConstIterator itEditor = m_editorToProperty.constBegin();
         itEditor != itcend; ++itEditor) {
        if (itEditor.key() == object) {
            QtProperty *property = itEditor.value();
            QtDoublePropertyManager *manager = q_ptr->propertyManager(property);
            if (!manager)
                return;
            manager->setValue(property, value);
            return;
        }
    }
}

/*! \class VistleDoubleSpinBoxFactory

    \brief The VistleDoubleSpinBoxFactory class provides VistleDoubleSpinBox
    widgets for properties created by QtDoublePropertyManager objects.

    \sa QtAbstractEditorFactory, QtDoublePropertyManager
*/

/*!
    Creates a factory with the given \a parent.
*/
VistleDoubleSpinBoxFactory::VistleDoubleSpinBoxFactory(QObject *parent)
: QtAbstractEditorFactory<QtDoublePropertyManager>(parent)
{
    d_ptr = new VistleDoubleSpinBoxFactoryPrivate();
    d_ptr->q_ptr = this;
}

/*!
    Destroys this factory, and all the widgets it has created.
*/
VistleDoubleSpinBoxFactory::~VistleDoubleSpinBoxFactory()
{
    qDeleteAll(d_ptr->m_editorToProperty.keys());
    delete d_ptr;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void VistleDoubleSpinBoxFactory::connectPropertyManager(QtDoublePropertyManager *manager)
{
    connect(manager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(slotPropertyChanged(QtProperty *, double)));
    connect(manager, SIGNAL(rangeChanged(QtProperty *, double, double)), this,
            SLOT(slotRangeChanged(QtProperty *, double, double)));
    connect(manager, SIGNAL(singleStepChanged(QtProperty *, double)), this,
            SLOT(slotSingleStepChanged(QtProperty *, double)));
    connect(manager, SIGNAL(decimalsChanged(QtProperty *, int)), this, SLOT(slotDecimalsChanged(QtProperty *, int)));
    connect(manager, SIGNAL(readOnlyChanged(QtProperty *, bool)), this, SLOT(slotReadOnlyChanged(QtProperty *, bool)));
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
QWidget *VistleDoubleSpinBoxFactory::createEditor(QtDoublePropertyManager *manager, QtProperty *property,
                                                  QWidget *parent)
{
    VistleDoubleSpinBox *editor = d_ptr->createEditor(property, parent);
    editor->setSingleStep(manager->singleStep(property));
    editor->setDecimals(manager->decimals(property));
    editor->setRange(manager->minimum(property), manager->maximum(property));
    editor->setValue(manager->value(property));
    editor->setKeyboardTracking(false);
    editor->setReadOnly(manager->isReadOnly(property));

    connect(editor, SIGNAL(valueChanged(double)), this, SLOT(slotSetValue(double)));
    connect(editor, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));
    return editor;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void VistleDoubleSpinBoxFactory::disconnectPropertyManager(QtDoublePropertyManager *manager)
{
    disconnect(manager, SIGNAL(valueChanged(QtProperty *, double)), this,
               SLOT(slotPropertyChanged(QtProperty *, double)));
    disconnect(manager, SIGNAL(rangeChanged(QtProperty *, double, double)), this,
               SLOT(slotRangeChanged(QtProperty *, double, double)));
    disconnect(manager, SIGNAL(singleStepChanged(QtProperty *, double)), this,
               SLOT(slotSingleStepChanged(QtProperty *, double)));
    disconnect(manager, SIGNAL(decimalsChanged(QtProperty *, int)), this, SLOT(slotDecimalsChanged(QtProperty *, int)));
    disconnect(manager, SIGNAL(readOnlyChanged(QtProperty *, bool)), this,
               SLOT(slotReadOnlyChanged(QtProperty *, bool)));
}


#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

#include "moc_vistledoubleeditorfactory.cpp"
