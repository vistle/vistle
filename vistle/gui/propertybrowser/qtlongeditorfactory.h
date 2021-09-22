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


#ifndef QTLONGEDITORFACTORY_H
#define QTLONGEDITORFACTORY_H

#include "qtpropertymanager.h"
#include "qtlongpropertymanager.h"

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QtLongSpinBoxFactoryPrivate;

class QT_QTPROPERTYBROWSER_EXPORT QtLongSpinBoxFactory: public QtAbstractEditorFactory<QtLongPropertyManager> {
    Q_OBJECT
public:
    QtLongSpinBoxFactory(QObject *parent = 0);
    ~QtLongSpinBoxFactory();

protected:
    void connectPropertyManager(QtLongPropertyManager *manager);
    QWidget *createEditor(QtLongPropertyManager *manager, QtProperty *property, QWidget *parent);
    void disconnectPropertyManager(QtLongPropertyManager *manager);

private:
    QtLongSpinBoxFactoryPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QtLongSpinBoxFactory)
    Q_DISABLE_COPY(QtLongSpinBoxFactory)
    Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, vistle::Integer, vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(QtProperty *, vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotReadOnlyChanged(QtProperty *, bool))
    Q_PRIVATE_SLOT(d_func(), void slotSetValue(vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtLongSliderFactoryPrivate;

class QT_QTPROPERTYBROWSER_EXPORT QtLongSliderFactory: public QtAbstractEditorFactory<QtLongPropertyManager> {
    Q_OBJECT
public:
    QtLongSliderFactory(QObject *parent = 0);
    ~QtLongSliderFactory();

protected:
    void connectPropertyManager(QtLongPropertyManager *manager);
    QWidget *createEditor(QtLongPropertyManager *manager, QtProperty *property, QWidget *parent);
    void disconnectPropertyManager(QtLongPropertyManager *manager);

private:
    QtLongSliderFactoryPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QtLongSliderFactory)
    Q_DISABLE_COPY(QtLongSliderFactory)
    Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, vistle::Integer, vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(QtProperty *, vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotSetValue(vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtLongScrollBarFactoryPrivate;

class QT_QTPROPERTYBROWSER_EXPORT QtLongScrollBarFactory: public QtAbstractEditorFactory<QtLongPropertyManager> {
    Q_OBJECT
public:
    QtLongScrollBarFactory(QObject *parent = 0);
    ~QtLongScrollBarFactory();

protected:
    void connectPropertyManager(QtLongPropertyManager *manager);
    QWidget *createEditor(QtLongPropertyManager *manager, QtProperty *property, QWidget *parent);
    void disconnectPropertyManager(QtLongPropertyManager *manager);

private:
    QtLongScrollBarFactoryPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QtLongScrollBarFactory)
    Q_DISABLE_COPY(QtLongScrollBarFactory)
    Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, vistle::Integer, vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(QtProperty *, vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotSetValue(vistle::Integer))
    Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

#endif
