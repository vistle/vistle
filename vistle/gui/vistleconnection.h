#ifndef VHANDLER_H
#define VHANDLER_H

#include "consts.h"

#include <userinterface/userinterface.h>

#include <QObject>

namespace gui {

class VistleConnection
{
public:
   VistleConnection(vistle::UserInterface &ui);
   static VistleConnection &the();

   void cancel();
   void operator()();

   vistle::UserInterface &ui() const;

   void sendMessage(const vistle::message::Message &msg) const;
   vistle::Parameter *getParameter(int id, QString name) const;
   template<class T>
   void setParameter(int id, QString name, const T &value) const;

private:
   vistle::UserInterface &m_ui;
   bool m_done = false;
   boost::mutex m_mutex;

   static VistleConnection *s_instance;
};

VistleConnection *vistle();

class VistleObserver: public QObject, public vistle::StateObserver
{
	Q_OBJECT

signals:
	void debug_s(QString debugMsg);
        void newModule_s(int moduleId, const boost::uuids::uuid &spawnUuid, QString moduleName);
	void deleteModule_s(int moduleId);
	void moduleStateChanged_s(int moduleId, int stateBits, ModuleStatus modChangeType);
	void newParameter_s(int moduleId, QString parameterName);
        void parameterValueChanged_s(int moduleId, QString parameterName);
	void newPort_s(int moduleId, QString portName);
	void newConnection_s(int fromId, QString fromName,
                   		 int toId, QString toName);
	void deleteConnection_s(int fromId, QString fromName,
                      		int toId, QString toName);

public:
    VistleObserver(QObject *parent=0);
   void newModule(int moduleId, const boost::uuids::uuid &spawnUuid, const std::string &moduleName);
	void deleteModule(int moduleId);
	void moduleStateChanged(int moduleId, int stateBits);
	void newParameter(int moduleId, const std::string &parameterName);
	void parameterValueChanged(int moduleId, const std::string &parameterName);
	void newPort(int moduleId, const std::string &portName);
	void newConnection(int fromId, const std::string &fromName,
                           int toId, const std::string &toName);
	void deleteConnection(int fromId, const std::string &fromName,
                          int toId, const std::string &toName);
};

template<class T>
void VistleConnection::setParameter(int id, QString name, const T &value) const
{
   vistle::ParameterBase<T> *p = dynamic_cast<vistle::ParameterBase<T> *>(getParameter(id, name));
   if (!p) {
      std::cerr << "parameter not of appropriate type: " << id << ":" << name.toStdString() << std::endl;
      return;
   }

   p->setValue(value);
   vistle::message::SetParameter set(id, name.toStdString(), p);
   sendMessage(set);
}

} //namespace gui
#endif // VHANDLER_H
