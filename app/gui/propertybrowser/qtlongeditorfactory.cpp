/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qteditorfactory.h"
#include "qtlongeditorfactory.h"
#include "qtpropertybrowserutils_p.h"
#include "qlongspinbox.h"

#include <QScrollBar>
#include <QLineEdit>
#include <QMap>

#include "propertyeditorfactory_p.h"

#if defined(Q_CC_MSVC)
#pragma warning(disable : 4786) /* MS VS 6: truncating debug info after 255 characters */
#endif

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif


// ------------ QtLongSpinBoxFactory

class QtLongSpinBoxFactoryPrivate: public EditorFactoryPrivate<QLongSpinBox> {
    QtLongSpinBoxFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtLongSpinBoxFactory)
public:
    void slotPropertyChanged(QtProperty *property, vistle::Integer value);
    void slotRangeChanged(QtProperty *property, vistle::Integer min, vistle::Integer max);
    void slotSingleStepChanged(QtProperty *property, vistle::Integer step);
    void slotReadOnlyChanged(QtProperty *property, bool readOnly);
    void slotSetValue(vistle::Integer value);
};

void QtLongSpinBoxFactoryPrivate::slotPropertyChanged(QtProperty *property, vistle::Integer value)
{
    if (!m_createdEditors.contains(property))
        return;
    QListIterator<QLongSpinBox *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QLongSpinBox *editor = itEditor.next();
        if (editor->value() != value) {
            editor->blockSignals(true);
            editor->setValue(value);
            editor->blockSignals(false);
        }
    }
}

void QtLongSpinBoxFactoryPrivate::slotRangeChanged(QtProperty *property, vistle::Integer min, vistle::Integer max)
{
    if (!m_createdEditors.contains(property))
        return;

    QtLongPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<QLongSpinBox *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QLongSpinBox *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setRange(min, max);
        editor->setValue(manager->value(property));
        editor->blockSignals(false);
    }
}

void QtLongSpinBoxFactoryPrivate::slotSingleStepChanged(QtProperty *property, vistle::Integer step)
{
    if (!m_createdEditors.contains(property))
        return;
    QListIterator<QLongSpinBox *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QLongSpinBox *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setSingleStep(step);
        editor->blockSignals(false);
    }
}

void QtLongSpinBoxFactoryPrivate::slotReadOnlyChanged(QtProperty *property, bool readOnly)
{
    if (!m_createdEditors.contains(property))
        return;

    QtLongPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<QLongSpinBox *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QLongSpinBox *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setReadOnly(readOnly);
        editor->blockSignals(false);
    }
}

void QtLongSpinBoxFactoryPrivate::slotSetValue(vistle::Integer value)
{
    QObject *object = q_ptr->sender();
    const QMap<QLongSpinBox *, QtProperty *>::ConstIterator ecend = m_editorToProperty.constEnd();
    for (QMap<QLongSpinBox *, QtProperty *>::ConstIterator itEditor = m_editorToProperty.constBegin();
         itEditor != ecend; ++itEditor) {
        if (itEditor.key() == object) {
            QtProperty *property = itEditor.value();
            QtLongPropertyManager *manager = q_ptr->propertyManager(property);
            if (!manager)
                return;
            manager->setValue(property, value);
            return;
        }
    }
}

/*!
    \class QtLongSpinBoxFactory

    \brief The QtLongSpinBoxFactory class provides QLongSpinBox widgets for
    properties created by QtIntPropertyManager objects.

    \sa QtAbstractEditorFactory, QtIntPropertyManager
*/

/*!
    Creates a factory with the given \a parent.
*/
QtLongSpinBoxFactory::QtLongSpinBoxFactory(QObject *parent): QtAbstractEditorFactory<QtLongPropertyManager>(parent)
{
    d_ptr = new QtLongSpinBoxFactoryPrivate();
    d_ptr->q_ptr = this;
}

/*!
    Destroys this factory, and all the widgets it has created.
*/
QtLongSpinBoxFactory::~QtLongSpinBoxFactory()
{
    qDeleteAll(d_ptr->m_editorToProperty.keys());
    delete d_ptr;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void QtLongSpinBoxFactory::connectPropertyManager(QtLongPropertyManager *manager)
{
    connect(manager, SIGNAL(valueChanged(QtProperty *, vistle::Integer)), this,
            SLOT(slotPropertyChanged(QtProperty *, vistle::Integer)));
    connect(manager, SIGNAL(rangeChanged(QtProperty *, vistle::Integer, vistle::Integer)), this,
            SLOT(slotRangeChanged(QtProperty *, vistle::Integer, vistle::Integer)));
    connect(manager, SIGNAL(singleStepChanged(QtProperty *, vistle::Integer)), this,
            SLOT(slotSingleStepChanged(QtProperty *, vistle::Integer)));
    connect(manager, SIGNAL(readOnlyChanged(QtProperty *, bool)), this, SLOT(slotReadOnlyChanged(QtProperty *, bool)));
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
QWidget *QtLongSpinBoxFactory::createEditor(QtLongPropertyManager *manager, QtProperty *property, QWidget *parent)
{
    QLongSpinBox *editor = d_ptr->createEditor(property, parent);
    editor->setSingleStep(manager->singleStep(property));
    editor->setRange(manager->minimum(property), manager->maximum(property));
    editor->setValue(manager->value(property));
    editor->setKeyboardTracking(false);
    editor->setReadOnly(manager->isReadOnly(property));

    connect(editor, SIGNAL(valueChanged(vistle::Integer)), this, SLOT(slotSetValue(vistle::Integer)));
    connect(editor, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));
    return editor;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void QtLongSpinBoxFactory::disconnectPropertyManager(QtLongPropertyManager *manager)
{
    disconnect(manager, SIGNAL(valueChanged(QtProperty *, vistle::Integer)), this,
               SLOT(slotPropertyChanged(QtProperty *, vistle::Integer)));
    disconnect(manager, SIGNAL(rangeChanged(QtProperty *, vistle::Integer, vistle::Integer)), this,
               SLOT(slotRangeChanged(QtProperty *, vistle::Integer, vistle::Integer)));
    disconnect(manager, SIGNAL(singleStepChanged(QtProperty *, vistle::Integer)), this,
               SLOT(slotSingleStepChanged(QtProperty *, vistle::Integer)));
    disconnect(manager, SIGNAL(readOnlyChanged(QtProperty *, bool)), this,
               SLOT(slotReadOnlyChanged(QtProperty *, bool)));
}

// QtLongSliderFactory

class QtLongSliderFactoryPrivate: public EditorFactoryPrivate<QSlider> {
    QtLongSliderFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtLongSliderFactory)
public:
    void slotPropertyChanged(QtProperty *property, vistle::Integer value);
    void slotRangeChanged(QtProperty *property, vistle::Integer min, vistle::Integer max);
    void slotSingleStepChanged(QtProperty *property, vistle::Integer step);
    void slotSetValue(vistle::Integer value);
};

void QtLongSliderFactoryPrivate::slotPropertyChanged(QtProperty *property, vistle::Integer value)
{
    if (!m_createdEditors.contains(property))
        return;
    QListIterator<QSlider *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QSlider *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setValue(value);
        editor->blockSignals(false);
    }
}

void QtLongSliderFactoryPrivate::slotRangeChanged(QtProperty *property, vistle::Integer min, vistle::Integer max)
{
    if (!m_createdEditors.contains(property))
        return;

    QtLongPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<QSlider *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QSlider *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setRange(min, max);
        editor->setValue(manager->value(property));
        editor->blockSignals(false);
    }
}

void QtLongSliderFactoryPrivate::slotSingleStepChanged(QtProperty *property, vistle::Integer step)
{
    if (!m_createdEditors.contains(property))
        return;
    QListIterator<QSlider *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QSlider *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setSingleStep(step);
        editor->blockSignals(false);
    }
}

void QtLongSliderFactoryPrivate::slotSetValue(vistle::Integer value)
{
    QObject *object = q_ptr->sender();
    const QMap<QSlider *, QtProperty *>::ConstIterator ecend = m_editorToProperty.constEnd();
    for (QMap<QSlider *, QtProperty *>::ConstIterator itEditor = m_editorToProperty.constBegin(); itEditor != ecend;
         ++itEditor) {
        if (itEditor.key() == object) {
            QtProperty *property = itEditor.value();
            QtLongPropertyManager *manager = q_ptr->propertyManager(property);
            if (!manager)
                return;
            manager->setValue(property, value);
            return;
        }
    }
}

/*!
    \class QtLongSliderFactory

    \brief The QtLongSliderFactory class provides QSlider widgets for
    properties created by QtIntPropertyManager objects.

    \sa QtAbstractEditorFactory, QtIntPropertyManager
*/

/*!
    Creates a factory with the given \a parent.
*/
QtLongSliderFactory::QtLongSliderFactory(QObject *parent): QtAbstractEditorFactory<QtLongPropertyManager>(parent)
{
    d_ptr = new QtLongSliderFactoryPrivate();
    d_ptr->q_ptr = this;
}

/*!
    Destroys this factory, and all the widgets it has created.
*/
QtLongSliderFactory::~QtLongSliderFactory()
{
    qDeleteAll(d_ptr->m_editorToProperty.keys());
    delete d_ptr;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void QtLongSliderFactory::connectPropertyManager(QtLongPropertyManager *manager)
{
    connect(manager, SIGNAL(valueChanged(QtProperty *, vistle::Integer)), this,
            SLOT(slotPropertyChanged(QtProperty *, vistle::Integer)));
    connect(manager, SIGNAL(rangeChanged(QtProperty *, vistle::Integer, vistle::Integer)), this,
            SLOT(slotRangeChanged(QtProperty *, vistle::Integer, vistle::Integer)));
    connect(manager, SIGNAL(singleStepChanged(QtProperty *, vistle::Integer)), this,
            SLOT(slotSingleStepChanged(QtProperty *, vistle::Integer)));
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
QWidget *QtLongSliderFactory::createEditor(QtLongPropertyManager *manager, QtProperty *property, QWidget *parent)
{
    QSlider *editor = new QSlider(Qt::Horizontal, parent);
    d_ptr->initializeEditor(property, editor);
    editor->setSingleStep(manager->singleStep(property));
    editor->setRange(manager->minimum(property), manager->maximum(property));
    editor->setValue(manager->value(property));

    connect(editor, SIGNAL(valueChanged(vistle::Integer)), this, SLOT(slotSetValue(vistle::Integer)));
    connect(editor, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));
    return editor;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void QtLongSliderFactory::disconnectPropertyManager(QtLongPropertyManager *manager)
{
    disconnect(manager, SIGNAL(valueChanged(QtProperty *, vistle::Integer)), this,
               SLOT(slotPropertyChanged(QtProperty *, vistle::Integer)));
    disconnect(manager, SIGNAL(rangeChanged(QtProperty *, vistle::Integer, vistle::Integer)), this,
               SLOT(slotRangeChanged(QtProperty *, vistle::Integer, vistle::Integer)));
    disconnect(manager, SIGNAL(singleStepChanged(QtProperty *, vistle::Integer)), this,
               SLOT(slotSingleStepChanged(QtProperty *, vistle::Integer)));
}

// QtLongSliderFactory

class QtLongScrollBarFactoryPrivate: public EditorFactoryPrivate<QScrollBar> {
    QtLongScrollBarFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtLongScrollBarFactory)
public:
    void slotPropertyChanged(QtProperty *property, vistle::Integer value);
    void slotRangeChanged(QtProperty *property, vistle::Integer min, vistle::Integer max);
    void slotSingleStepChanged(QtProperty *property, vistle::Integer step);
    void slotSetValue(vistle::Integer value);
};

void QtLongScrollBarFactoryPrivate::slotPropertyChanged(QtProperty *property, vistle::Integer value)
{
    if (!m_createdEditors.contains(property))
        return;

    QListIterator<QScrollBar *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QScrollBar *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setValue(value);
        editor->blockSignals(false);
    }
}

void QtLongScrollBarFactoryPrivate::slotRangeChanged(QtProperty *property, vistle::Integer min, vistle::Integer max)
{
    if (!m_createdEditors.contains(property))
        return;

    QtLongPropertyManager *manager = q_ptr->propertyManager(property);
    if (!manager)
        return;

    QListIterator<QScrollBar *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QScrollBar *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setRange(min, max);
        editor->setValue(manager->value(property));
        editor->blockSignals(false);
    }
}

void QtLongScrollBarFactoryPrivate::slotSingleStepChanged(QtProperty *property, vistle::Integer step)
{
    if (!m_createdEditors.contains(property))
        return;
    QListIterator<QScrollBar *> itEditor(m_createdEditors[property]);
    while (itEditor.hasNext()) {
        QScrollBar *editor = itEditor.next();
        editor->blockSignals(true);
        editor->setSingleStep(step);
        editor->blockSignals(false);
    }
}

void QtLongScrollBarFactoryPrivate::slotSetValue(vistle::Integer value)
{
    QObject *object = q_ptr->sender();
    const QMap<QScrollBar *, QtProperty *>::ConstIterator ecend = m_editorToProperty.constEnd();
    for (QMap<QScrollBar *, QtProperty *>::ConstIterator itEditor = m_editorToProperty.constBegin(); itEditor != ecend;
         ++itEditor)
        if (itEditor.key() == object) {
            QtProperty *property = itEditor.value();
            QtLongPropertyManager *manager = q_ptr->propertyManager(property);
            if (!manager)
                return;
            manager->setValue(property, value);
            return;
        }
}

/*!
    \class QtLongScrollBarFactory

    \brief The QtLongScrollBarFactory class provides QScrollBar widgets for
    properties created by QtIntPropertyManager objects.

    \sa QtAbstractEditorFactory, QtIntPropertyManager
*/

/*!
    Creates a factory with the given \a parent.
*/
QtLongScrollBarFactory::QtLongScrollBarFactory(QObject *parent): QtAbstractEditorFactory<QtLongPropertyManager>(parent)
{
    d_ptr = new QtLongScrollBarFactoryPrivate();
    d_ptr->q_ptr = this;
}

/*!
    Destroys this factory, and all the widgets it has created.
*/
QtLongScrollBarFactory::~QtLongScrollBarFactory()
{
    qDeleteAll(d_ptr->m_editorToProperty.keys());
    delete d_ptr;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void QtLongScrollBarFactory::connectPropertyManager(QtLongPropertyManager *manager)
{
    connect(manager, SIGNAL(valueChanged(QtProperty *, vistle::Integer)), this,
            SLOT(slotPropertyChanged(QtProperty *, vistle::Integer)));
    connect(manager, SIGNAL(rangeChanged(QtProperty *, vistle::Integer, vistle::Integer)), this,
            SLOT(slotRangeChanged(QtProperty *, vistle::Integer, vistle::Integer)));
    connect(manager, SIGNAL(singleStepChanged(QtProperty *, vistle::Integer)), this,
            SLOT(slotSingleStepChanged(QtProperty *, vistle::Integer)));
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
QWidget *QtLongScrollBarFactory::createEditor(QtLongPropertyManager *manager, QtProperty *property, QWidget *parent)
{
    QScrollBar *editor = new QScrollBar(Qt::Horizontal, parent);
    d_ptr->initializeEditor(property, editor);
    editor->setSingleStep(manager->singleStep(property));
    editor->setRange(manager->minimum(property), manager->maximum(property));
    editor->setValue(manager->value(property));
    connect(editor, SIGNAL(valueChanged(vistle::Integer)), this, SLOT(slotSetValue(vistle::Integer)));
    connect(editor, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));
    return editor;
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void QtLongScrollBarFactory::disconnectPropertyManager(QtLongPropertyManager *manager)
{
    disconnect(manager, SIGNAL(valueChanged(QtProperty *, vistle::Integer)), this,
               SLOT(slotPropertyChanged(QtProperty *, vistle::Integer)));
    disconnect(manager, SIGNAL(rangeChanged(QtProperty *, vistle::Integer, vistle::Integer)), this,
               SLOT(slotRangeChanged(QtProperty *, vistle::Integer, vistle::Integer)));
    disconnect(manager, SIGNAL(singleStepChanged(QtProperty *, vistle::Integer)), this,
               SLOT(slotSingleStepChanged(QtProperty *, vistle::Integer)));
}

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

#include "moc_qtlongeditorfactory.cpp"
