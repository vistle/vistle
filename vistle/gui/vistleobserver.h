#ifndef VISTLEOBSERVER_H
#define VISTLEOBSERVER_H

#include <QObject>

#include <boost/thread/recursive_mutex.hpp>

#include <userinterface/userinterface.h>
#include <userinterface/vistleconnection.h>

#include "module.h"

namespace gui {

class VistleObserver: public QObject, public vistle::StateObserver
{
	Q_OBJECT

signals:
void moduleAvailable_s(QString name);
        void newModule_s(int moduleId, const boost::uuids::uuid &spawnUuid, QString moduleName);
	void deleteModule_s(int moduleId);
   void moduleStateChanged_s(int moduleId, int stateBits);
	void newParameter_s(int moduleId, QString parameterName);
        void parameterValueChanged_s(int moduleId, QString parameterName);
        void parameterChoicesChanged_s(int moduleId, QString parameterName);
   void newPort_s(int moduleId, QString portName);
	void newConnection_s(int fromId, QString fromName,
                   		 int toId, QString toName);
	void deleteConnection_s(int fromId, QString fromName,
                      		int toId, QString toName);
   void modified(bool state);

   void info_s(QString msg, int type);

   void quit_s();

public:
   VistleObserver(QObject *parent=0);
   void moduleAvailable(const std::string &name);
   void newModule(int moduleId, const boost::uuids::uuid &spawnUuid, const std::string &moduleName);
	void deleteModule(int moduleId);
	void moduleStateChanged(int moduleId, int stateBits);
	void newParameter(int moduleId, const std::string &parameterName);
	void parameterValueChanged(int moduleId, const std::string &parameterName);
   void parameterChoicesChanged(int moduleId, const std::string &parameterName);
   void newPort(int moduleId, const std::string &portName);
	void newConnection(int fromId, const std::string &fromName,
                           int toId, const std::string &toName);
	void deleteConnection(int fromId, const std::string &fromName,
                          int toId, const std::string &toName);

   void info(const std::string &text, vistle::message::SendText::TextType textType, int senderId, int senderRank, vistle::message::Message::Type refType, const vistle::message::Message::uuid_t &refUuid);

   void quitRequested();

   void incModificationCount();
   void resetModificationCount();

private:
   std::map<int, QString> m_moduleNames;
};

} //namespace gui
#endif
