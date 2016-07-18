#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <map>

#include <QtButtonPropertyBrowser>

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
   void deleteParameter(int moduleId, QString parameterName);
   void parameterValueChanged(int moduleId, QString parameterName);
   void parameterChoicesChanged(int moduleId, QString parameterName);
   void propertyChanged(QtProperty *prop);

private:
   int m_moduleId;
   vistle::VistleConnection *m_vistle;

   QtProperty *m_internalGroup;
   std::map<QString, QtProperty *> m_groups;

   QtGroupPropertyManager *m_groupManager;
   QtBoolPropertyManager *m_boolManager;
   QtLongPropertyManager *m_intManager;
   QtDoublePropertyManager *m_floatManager;
   QtStringPropertyManager *m_stringManager;
   QtEnumPropertyManager *m_stringChoiceManager;
   QtEnumPropertyManager *m_intChoiceManager;
   QtVectorPropertyManager *m_vectorManager;
   QtLongVectorPropertyManager *m_intVectorManager;

   std::map<QString, QtProperty *> m_paramToProp;
   std::map<QtProperty *, QString> m_propToParam;

   bool m_ignorePropertyChanges;
};

} //namespace gui

#endif
