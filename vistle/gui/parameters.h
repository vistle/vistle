#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <map>

#include <QtButtonPropertyBrowser>

class QtProperty;

class QtGroupPropertyManager;
class QtBoolPropertyManager;
class QtIntPropertyManager;
class QtDoublePropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;

namespace vistle {
class VistleConnection;
}

namespace gui {

class VistleObserver;

class Parameters: public QtButtonPropertyBrowser
{
   Q_OBJECT

public:
   typedef QtButtonPropertyBrowser PropertyBrowser;

   Parameters(QWidget * parent = 0, Qt::WindowFlags f = 0);
   void setVistleObserver(VistleObserver *observer);
   void setVistleConnection(vistle::VistleConnection *conn);
   void setModule(int id);

private slots:
   void newParameter(int moduleId, QString parameterName);
   void parameterValueChanged(int moduleId, QString parameterName);
   void parameterChoicesChanged(int moduleId, QString parameterName);
   void propertyChanged(QtProperty *prop);

private:
   int m_moduleId = 0;
   vistle::VistleConnection *m_vistle = nullptr;

   QtProperty *m_internalGroup = nullptr;

   QtGroupPropertyManager *m_groupManager = nullptr;
   QtBoolPropertyManager *m_boolManager = nullptr;
   QtIntPropertyManager *m_intManager = nullptr;
   QtDoublePropertyManager *m_floatManager = nullptr;
   QtStringPropertyManager *m_stringManager = nullptr;
   QtEnumPropertyManager *m_stringChoiceManager = nullptr;
   QtEnumPropertyManager *m_intChoiceManager = nullptr;

   std::map<QString, QtProperty *> m_paramToProp;
   std::map<QtProperty *, QString> m_propToParam;
};

} //namespace gui

#endif
