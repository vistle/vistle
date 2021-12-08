#ifndef VISTLEOBSERVER_H
#define VISTLEOBSERVER_H

#include <QObject>

#include <vistle/userinterface/userinterface.h>
#include <vistle/userinterface/vistleconnection.h>

#include "module.h"
namespace gui {

class VistleObserver: public QObject, public vistle::StateObserver {
    Q_OBJECT

signals:
    void newHub_s(int hub, QString name, int nranks, QString address, QString logname, QString realname);
    void deleteHub_s(int hub);
    void moduleAvailable_s(int hub, QString name, QString path, QString description);
    void newModule_s(int moduleId, const boost::uuids::uuid &spawnUuid, QString moduleName);
    void deleteModule_s(int moduleId);
    void moduleStateChanged_s(int moduleId, int stateBits);
    void newParameter_s(int moduleId, QString parameterName);
    void deleteParameter_s(int moduleId, QString parameterName);
    void parameterValueChanged_s(int moduleId, QString parameterName);
    void parameterChoicesChanged_s(int moduleId, QString parameterName);
    void newPort_s(int moduleId, QString portName);
    void deletePort_s(int moduleId, QString portName);
    void newConnection_s(int fromId, QString fromName, int toId, QString toName);
    void deleteConnection_s(int fromId, QString fromName, int toId, QString toName);
    void modified(bool state);

    void info_s(QString msg, int type);
    void status_s(int id, QString msg, int prio);
    void moduleStatus_s(int id, QString msg, int prio);

    void loadedWorkflowChanged_s(QString file);
    void sessionUrlChanged_s(QString url);

    void quit_s();

    void screenshot_s(QString msg);

public:
    VistleObserver(QObject *parent = 0);
    void newHub(int hub, const std::string &name, int nranks, const std::string &address, const std::string &logname,
                const std::string &realname) override;
    void deleteHub(int hub) override;
    void moduleAvailable(const vistle::AvailableModule &mod) override;
    void newModule(int moduleId, const boost::uuids::uuid &spawnUuid, const std::string &moduleName) override;
    void deleteModule(int moduleId) override;
    void moduleStateChanged(int moduleId, int stateBits) override;
    void newParameter(int moduleId, const std::string &parameterName) override;
    void deleteParameter(int moduleId, const std::string &parameterName) override;
    void parameterValueChanged(int moduleId, const std::string &parameterName) override;
    void parameterChoicesChanged(int moduleId, const std::string &parameterName) override;
    void newPort(int moduleId, const std::string &portName) override;
    void deletePort(int moduleId, const std::string &portName) override;
    void newConnection(int fromId, const std::string &fromName, int toId, const std::string &toName) override;
    void deleteConnection(int fromId, const std::string &fromName, int toId, const std::string &toName) override;

    void info(const std::string &text, vistle::message::SendText::TextType textType, int senderId, int senderRank,
              vistle::message::Type refType, const vistle::message::uuid_t &refUuid) override;
    void status(int id, const std::string &text, vistle::message::UpdateStatus::Importance priority) override;
    void updateStatus(int id, const std::string &text, vistle::message::UpdateStatus::Importance priority) override;

    void quitRequested() override;

    void incModificationCount() override;
    void resetModificationCount() override;

    void loadedWorkflowChanged(const std::string &filename) override;
    void sessionUrlChanged(const std::string &url) override;

    void message(const vistle::message::Message &msg, vistle::buffer *payload) override;

private:
    std::map<int, QString> m_moduleNames;
};

} //namespace gui
#endif
