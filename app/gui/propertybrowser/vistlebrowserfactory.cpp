#include "vistlebrowserfactory.h"

#include "qteditorfactory.h"
#include "qtpropertybrowserutils_p.h"
#include "propertyeditorfactory_p.h"

#include <QLineEdit>

#include "vistlebrowseredit.h"
#include <vistle/userinterface/userinterface.h>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

// VistleBrowserFactory

class VistleBrowserFactoryPrivate: public EditorFactoryPrivate<VistleBrowserEdit> {
    VistleBrowserFactory *q_ptr;
    Q_DECLARE_PUBLIC(VistleBrowserFactory)
public:
    VistleBrowserFactoryPrivate();
    vistle::UserInterface *m_ui = nullptr;

    void slotPropertyChanged(QtProperty *property, const QString &value);
    void slotModuleIdChanged(QtProperty *, int);
    void slotFiltersChanged(QtProperty *property, const QString &filters);
    void slotRegExpChanged(QtProperty *property, const QRegularExpression &regExp);
    void slotSetValue(const QString &value);
    void slotEchoModeChanged(QtProperty *, int);
    void slotReadOnlyChanged(QtProperty *, bool);
    void slotFileModeChanged(QtProperty *, int);
    void slotTitleChanged(QtProperty *, const QString &title);
};

VistleBrowserFactoryPrivate::VistleBrowserFactoryPrivate()
{}

void VistleBrowserFactoryPrivate::slotPropertyChanged(QtProperty *property, const QString &value)
{
    if (!m_createdEditors.contains(property))
        return;

    QListIterator<VistleBrowserEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        VistleBrowserEdit *editor = itEditor.next();
        if (editor->text() != value) {
            editor->blockSignals(true);
            editor->setText(value);
            editor->blockSignals(false);
        }
    }
}

void VistleBrowserFactoryPrivate::slotFiltersChanged(QtProperty *property, const QString &filters)
{
    if (!m_createdEditors.contains(property))
        return;

    VistleBrowserPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<VistleBrowserEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        VistleBrowserEdit *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setFilters(filters);
        editor->blockSignals(false);
    }
}

void VistleBrowserFactoryPrivate::slotRegExpChanged(QtProperty *property, const QRegularExpression &regExp)
{
    if (!m_createdEditors.contains(property))
        return;

    VistleBrowserPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<VistleBrowserEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        VistleBrowserEdit *editor = itEditor.next();
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

void VistleBrowserFactoryPrivate::slotModuleIdChanged(QtProperty *property, int id)
{
    if (!m_createdEditors.contains(property))
        return;

    VistleBrowserPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<VistleBrowserEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        VistleBrowserEdit *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setModuleId(id);
        editor->blockSignals(false);
    }
}

void VistleBrowserFactoryPrivate::slotEchoModeChanged(QtProperty *property, int echoMode)
{
    if (!m_createdEditors.contains(property))
        return;

    VistleBrowserPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<VistleBrowserEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        VistleBrowserEdit *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setEchoMode((EchoMode)echoMode);
        editor->blockSignals(false);
    }
}

void VistleBrowserFactoryPrivate::slotReadOnlyChanged(QtProperty *property, bool readOnly)
{
    if (!m_createdEditors.contains(property))
        return;

    VistleBrowserPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<VistleBrowserEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        VistleBrowserEdit *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setReadOnly(readOnly);
        editor->blockSignals(false);
    }
}

void VistleBrowserFactoryPrivate::slotFileModeChanged(QtProperty *property, int fileMode)
{
    if (!m_createdEditors.contains(property))
        return;

    VistleBrowserPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<VistleBrowserEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        VistleBrowserEdit *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setFileMode((FileMode)fileMode);
        editor->blockSignals(false);
    }
}

void VistleBrowserFactoryPrivate::slotTitleChanged(QtProperty *property, const QString &title)
{
    if (!m_createdEditors.contains(property))
        return;

    VistleBrowserPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<VistleBrowserEdit *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        VistleBrowserEdit *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setTitle(title);
        editor->blockSignals(false);
    }
}

void VistleBrowserFactoryPrivate::slotSetValue(const QString &value)
{
    QObject *object = q_ptr->sender();
    const QMap<VistleBrowserEdit *, QtProperty *>::ConstIterator ecend = m_editorToProperty.constEnd();
    for (QMap<VistleBrowserEdit *, QtProperty *>::ConstIterator itEditor = m_editorToProperty.constBegin();
         itEditor != ecend; ++itEditor)
        if (itEditor.key() == object) {
            QtProperty *property = itEditor.value();
            VistleBrowserPropertyManager *manager = q_ptr->propertyManager(property);
            if (!manager)
                return;
            manager->setValue(property, value);
            return;
        }
}


/*!
    \class VistleBrowserFactory

    \brief The VistleBrowserFactory class provides VistleBrowserEdit widgets for
    properties created by VistleBrowserPropertyManager objects.

    \sa QtAbstractEditorFactory, VistleBrowserPropertyManager
*/

/*!
    Creates a factory with the given \a parent.
*/
VistleBrowserFactory::VistleBrowserFactory(QObject *parent)
: QtAbstractEditorFactory<VistleBrowserPropertyManager>(parent)
{
    d_ptr = new VistleBrowserFactoryPrivate();
    d_ptr->q_ptr = this;
}

/*!
    Destroys this factory, and all the widgets it has created.
*/
VistleBrowserFactory::~VistleBrowserFactory()
{
    qDeleteAll(d_ptr->m_editorToProperty.keys());
    delete d_ptr;
}

void VistleBrowserFactory::setUi(vistle::UserInterface *ui)
{
    d_ptr->m_ui = ui;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void VistleBrowserFactory::connectPropertyManager(VistleBrowserPropertyManager *manager)
{
    connect(manager, SIGNAL(valueChanged(QtProperty *, const QString &)), this,
            SLOT(slotPropertyChanged(QtProperty *, const QString &)));
    connect(manager, SIGNAL(moduleIdChanged(QtProperty *, int)), this, SLOT(slotModuleIdChanged(QtProperty *, int)));
    connect(manager, SIGNAL(filtersChanged(QtProperty *, const QString &)), this,
            SLOT(slotFiltersChanged(QtProperty *, const QString &)));
    connect(manager, SIGNAL(regExpChanged(QtProperty *, const QRegularExpression &)), this,
            SLOT(slotRegExpChanged(QtProperty *, const QRegularExpression &)));
    connect(manager, SIGNAL(echoModeChanged(QtProperty *, int)), this, SLOT(slotEchoModeChanged(QtProperty *, int)));
    connect(manager, SIGNAL(readOnlyChanged(QtProperty *, bool)), this, SLOT(slotReadOnlyChanged(QtProperty *, bool)));
    connect(manager, SIGNAL(fileModeChanged(QtProperty *, int)), this, SLOT(slotFileModeChanged(QtProperty *, int)));
    connect(manager, SIGNAL(titleChanged(QtProperty *, const QString &)), this,
            SLOT(slotTitleChanged(QtProperty *, const QString &)));
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
QWidget *VistleBrowserFactory::createEditor(VistleBrowserPropertyManager *manager, QtProperty *property,
                                            QWidget *parent)
{
    VistleBrowserEdit *editor = d_ptr->createEditor(property, parent);
    editor->setUi(d_ptr->m_ui);
    editor->setModuleId(manager->moduleId(property));
    editor->setFilters(manager->filters(property));
    editor->setEchoMode((EchoMode)manager->echoMode(property));
    editor->setReadOnly(manager->isReadOnly(property));
    editor->setFileMode((FileMode)manager->fileMode(property));
    editor->setTitle(manager->title(property));
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
    connect(editor, &VistleBrowserEdit::editingFinished,
            [manager, property, editor]() { manager->setValue(property, editor->text()); });
#endif
    connect(editor, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));
    return editor;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void VistleBrowserFactory::disconnectPropertyManager(VistleBrowserPropertyManager *manager)
{
    disconnect(manager, SIGNAL(valueChanged(QtProperty *, const QString &)), this,
               SLOT(slotPropertyChanged(QtProperty *, const QString &)));
    disconnect(manager, SIGNAL(moduleIdChanged(QtProperty *, int)), this, SLOT(slotModuleIdChanged(QtProperty *, int)));
    disconnect(manager, SIGNAL(filtersChanged(QtProperty *, const QString &)), this,
               SLOT(slotFiltersChanged(QtProperty *, const QString &)));
    disconnect(manager, SIGNAL(regExpChanged(QtProperty *, const QRegularExpression &)), this,
               SLOT(slotRegExpChanged(QtProperty *, const QRegularExpression &)));
    disconnect(manager, SIGNAL(echoModeChanged(QtProperty *, int)), this, SLOT(slotEchoModeChanged(QtProperty *, int)));
    disconnect(manager, SIGNAL(readOnlyChanged(QtProperty *, bool)), this,
               SLOT(slotReadOnlyChanged(QtProperty *, bool)));
    disconnect(manager, SIGNAL(fileModeChanged(QtProperty *, int)), this, SLOT(slotFileModeChanged(QtProperty *, int)));
    disconnect(manager, SIGNAL(titleChanged(QtProperty *, const QString &)), this,
               SLOT(slotTitleChanged(QtProperty *, const QString &)));
}

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

#include "moc_vistlebrowserfactory.cpp"
