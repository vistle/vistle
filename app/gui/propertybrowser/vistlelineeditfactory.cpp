#include "vistlelineeditfactory.h"

#include "qteditorfactory.h"
#include "qtpropertybrowserutils_p.h"
#include "propertyeditorfactory_p.h"

#include <QLineEdit>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

// VistleLineEditFactory

class VistleLineEditFactoryPrivate: public EditorFactoryPrivate<QLineEdit> {
    VistleLineEditFactory *q_ptr;
    Q_DECLARE_PUBLIC(VistleLineEditFactory)
public:
    void slotPropertyChanged(QtProperty *property, const QString &value);
    void slotRegExpChanged(QtProperty *property, const QRegularExpression &regExp);
    void slotSetValue(const QString &value);
    void slotEchoModeChanged(QtProperty *, int);
    void slotReadOnlyChanged(QtProperty *, bool);
};

void VistleLineEditFactoryPrivate::slotPropertyChanged(QtProperty *property, const QString &value)
{
    if (!m_createdEditors.contains(property))
        return;

    QListIterator<QLineEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QLineEdit *editor = itEditor.next();
        if (editor->text() != value) {
            editor->blockSignals(true);
            editor->setText(value);
            editor->blockSignals(false);
        }
    }
}

void VistleLineEditFactoryPrivate::slotRegExpChanged(QtProperty *property, const QRegularExpression &regExp)
{
    if (!m_createdEditors.contains(property))
        return;

    QtStringPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<QLineEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QLineEdit *editor = itEditor.next();
        editor->blockSignals(true);
        const QValidator *oldValidator = editor->validator();
        QValidator *newValidator = 0;
        if (regExp.isValid()) {
            newValidator = new QRegularExpressionValidator(regExp, editor);
        }
        editor->setValidator(newValidator);
        if (oldValidator)
            delete oldValidator;
        editor->blockSignals(false);
    }
}

void VistleLineEditFactoryPrivate::slotEchoModeChanged(QtProperty *property, int echoMode)
{
    if (!m_createdEditors.contains(property))
        return;

    QtStringPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<QLineEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QLineEdit *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setEchoMode((EchoMode)echoMode);
        editor->blockSignals(false);
    }
}

void VistleLineEditFactoryPrivate::slotReadOnlyChanged(QtProperty *property, bool readOnly)
{
    if (!m_createdEditors.contains(property))
        return;

    QtStringPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<QLineEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QLineEdit *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setReadOnly(readOnly);
        editor->blockSignals(false);
    }
}

void VistleLineEditFactoryPrivate::slotSetValue(const QString &value)
{
    QObject *object = q_ptr->sender();
    const QMap<QLineEdit *, QtProperty *>::ConstIterator ecend = m_editorToProperty.constEnd();
    for (QMap<QLineEdit *, QtProperty *>::ConstIterator itEditor = m_editorToProperty.constBegin(); itEditor != ecend;
         ++itEditor)
        if (itEditor.key() == object) {
            QtProperty *property = itEditor.value();
            QtStringPropertyManager *manager = q_ptr->propertyManager(property);
            if (!manager)
                return;
            manager->setValue(property, value);
            return;
        }
}


/*!
    \class VistleLineEditFactory

    \brief The VistleLineEditFactory class provides QLineEdit widgets for
    properties created by QtStringPropertyManager objects.

    \sa QtAbstractEditorFactory, QtStringPropertyManager
*/

/*!
    Creates a factory with the given \a parent.
*/
VistleLineEditFactory::VistleLineEditFactory(QObject *parent): QtAbstractEditorFactory<QtStringPropertyManager>(parent)
{
    d_ptr = new VistleLineEditFactoryPrivate();
    d_ptr->q_ptr = this;
}

/*!
    Destroys this factory, and all the widgets it has created.
*/
VistleLineEditFactory::~VistleLineEditFactory()
{
    qDeleteAll(d_ptr->m_editorToProperty.keys());
    delete d_ptr;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void VistleLineEditFactory::connectPropertyManager(QtStringPropertyManager *manager)
{
    connect(manager, SIGNAL(valueChanged(QtProperty *, const QString &)), this,
            SLOT(slotPropertyChanged(QtProperty *, const QString &)));
    connect(manager, SIGNAL(regExpChanged(QtProperty *, const QRegularExpression &)), this,
            SLOT(slotRegExpChanged(QtProperty *, const QRegularExpression &)));
    connect(manager, SIGNAL(echoModeChanged(QtProperty *, int)), this, SLOT(slotEchoModeChanged(QtProperty *, int)));
    connect(manager, SIGNAL(readOnlyChanged(QtProperty *, bool)), this, SLOT(slotReadOnlyChanged(QtProperty *, bool)));
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
QWidget *VistleLineEditFactory::createEditor(QtStringPropertyManager *manager, QtProperty *property, QWidget *parent)
{
    QLineEdit *editor = d_ptr->createEditor(property, parent);
    editor->setEchoMode((EchoMode)manager->echoMode(property));
    editor->setReadOnly(manager->isReadOnly(property));
    QRegularExpression regExp = manager->regExp(property);
    if (regExp.isValid()) {
        QValidator *validator = new QRegularExpressionValidator(regExp, editor);
        editor->setValidator(validator);
    }
    editor->setText(manager->value(property));

#if 0
    connect(editor, SIGNAL(textChanged(const QString &)),
                this, SLOT(slotSetValue(const QString &)));
#else
    connect(editor, &QLineEdit::editingFinished,
            [manager, property, editor]() { manager->setValue(property, editor->text()); });
#endif
    connect(editor, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));
    return editor;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void VistleLineEditFactory::disconnectPropertyManager(QtStringPropertyManager *manager)
{
    disconnect(manager, SIGNAL(valueChanged(QtProperty *, const QString &)), this,
               SLOT(slotPropertyChanged(QtProperty *, const QString &)));
    disconnect(manager, SIGNAL(regExpChanged(QtProperty *, const QRegularExpression &)), this,
               SLOT(slotRegExpChanged(QtProperty *, const QRegularExpression &)));
    disconnect(manager, SIGNAL(echoModeChanged(QtProperty *, int)), this, SLOT(slotEchoModeChanged(QtProperty *, int)));
    disconnect(manager, SIGNAL(readOnlyChanged(QtProperty *, bool)), this,
               SLOT(slotReadOnlyChanged(QtProperty *, bool)));
}

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

#include "moc_vistlelineeditfactory.cpp"
