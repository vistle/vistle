#ifndef VISTLE_GUI_PARAMETERS_H
#define VISTLE_GUI_PARAMETERS_H

#include <map>

#include "propertybrowser/vistlebuttonpropertybrowser.h"

class QtProperty;

class QtGroupPropertyManager;
class QtBoolPropertyManager;
class QtIntPropertyManager;
class QtLongPropertyManager;
class QtDoublePropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class QtVectorPropertyManager;
class QtLongVectorPropertyManager;
class VistleBrowserPropertyManager;
class VistleBrowserFactory;

namespace vistle {
class VistleConnection;
}

namespace gui {

class VistleObserver;

class Parameters: public VistleButtonPropertyBrowser {
    Q_OBJECT

public:
    typedef VistleButtonPropertyBrowser PropertyBrowser;

    Parameters(QWidget *parent = 0, Qt::WindowFlags f = Qt::WindowFlags());
    void setVistleObserver(VistleObserver *observer);
    void setVistleConnection(vistle::VistleConnection *conn);
    void setModule(int id);
    void setConnectedParameters();
    static const char *mimeFormat();

private slots:
    void newParameter(int moduleId, QString parameterName);
    void deleteParameter(int moduleId, QString parameterName);
    void parameterValueChanged(int moduleId, QString parameterName);
    void parameterChoicesChanged(int moduleId, QString parameterName);
    void propertyChanged(QtProperty *prop);

private:
    QString propertyToName(QtProperty *prop) const;
    void setParameterEnabled(QtProperty *prop, bool state);
    bool getExpandedState(QString name, bool &state);
    void addItemWithProperty(QtBrowserItem *item, QtProperty *prop);

    vistle::VistleConnection *m_vistle;

    std::map<QString, QtProperty *> m_groups;
    std::map<QtProperty *, QString> m_propToGroup;

    QtGroupPropertyManager *m_groupManager;
    QtBoolPropertyManager *m_boolManager;
    QtLongPropertyManager *m_intManager;
    QtDoublePropertyManager *m_floatManager;
    QtStringPropertyManager *m_stringManager;
    VistleBrowserPropertyManager *m_browserManager;
    QtEnumPropertyManager *m_stringChoiceManager;
    QtEnumPropertyManager *m_intChoiceManager;
    QtVectorPropertyManager *m_vectorManager;
    QtLongVectorPropertyManager *m_intVectorManager;

    VistleBrowserFactory *m_browserFactory;

    std::map<QString, QtProperty *> m_paramToProp;
    std::map<QtProperty *, QString> m_propToParam;
    std::map<QtBrowserItem *, QtProperty *> m_itemToProp;

    bool m_ignorePropertyChanges = false;

    std::map<int, std::map<QString, bool>> m_expandedState;
};

} //namespace gui

#endif
