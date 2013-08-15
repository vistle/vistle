#ifndef VHANDLER_H
#define VHANDLER_H

#include "consts.h"

#include <userinterface/userinterface.h>

#include <QObject>

namespace gui {

class Handler : public QObject
{
public:
	Handler();
};

class UiRunner
{
public:
    UiRunner(vistle::UserInterface &ui);

    void cancel();
    void operator()();
    vistle::UserInterface &m_ui;

private:
	bool m_done = false;
    boost::mutex m_mutex;
};

class VistleObserver: public QObject, public vistle::StateObserver
{
	Q_OBJECT

signals:
	void debug_s(QString debugMsg);
        void newModule_s(int moduleId, const boost::uuids::uuid &spawnUuid, QString moduleName);
	void deleteModule_s(int moduleId);
	void moduleStateChanged_s(int moduleId, int stateBits, ModuleStatus modChangeType);
	void newParameter_s(int moduleId, QString parameterName);
	void parameterValueChanged_s(int moduleId, QString &parameterName);
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

} //namespace gui
#endif // VHANDLER_H
