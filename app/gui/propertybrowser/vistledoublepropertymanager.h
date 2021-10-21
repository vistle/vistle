#ifndef VISTLE_DOUBLE_PROPERTY_MANAGER
#define VISTLE_DOUBLE_PROPERTY_MANAGER

#include <QtDoublePropertyManager>

class VistleDoublePropertyManagerPrivate;

class QT_QTPROPERTYBROWSER_EXPORT VistleDoublePropertyManager: public QtDoublePropertyManager {
    Q_OBJECT
public:
    VistleDoublePropertyManager(QObject *parent = 0);

protected:
    QString valueText(const QtProperty *property) const;

#if 0
private:
    VistleDoublePropertyManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(VistleDoublePropertyManager)
    Q_DISABLE_COPY(VistleDoublePropertyManager)
#endif
};

#endif
