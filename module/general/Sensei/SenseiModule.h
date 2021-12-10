#ifndef SENSEI_CONTROL_MODULE_H
#define SENSEI_CONTROL_MODULE_H

#include <vistle/insitu/sensei/intOption.h>

#include <vistle/core/message.h>
#include <vistle/core/messagequeue.h>

#include <vistle/insitu/message/InSituMessage.h>
#include <vistle/insitu/message/ShmMessage.h>
#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/module/module.h>

#include <thread>
#include <chrono>

namespace vistle {
namespace insitu {
namespace sensei {

class SenseiModule: public vistle::Module {
public:
    typedef std::lock_guard<std::mutex> Guard;

    SenseiModule(const std::string &name, int moduleID, mpi::communicator comm);
    ~SenseiModule();

private:
    insitu::message::InSituShmMessage m_messageHandler;

    vistle::StringParameter *m_filePath = nullptr;
    vistle::IntParameter *m_deleteShm = nullptr;
    bool m_connectedToSim = false; // whether the socket connection to the engine is running
    std::map<std::string, vistle::Port *> m_outputPorts; // output ports for the data the simulation offers
    std::set<vistle::Parameter *> m_commandParameter; // buttons to trigger simulation commands
    std::unique_ptr<vistle::message::MessageQueue>
        m_receiveFromSimMessageQueue; // receives vistle messages that will be passed through to manager
    size_t m_ownExecutionCounter = 0;

    //...................................................................................

    std::map<const vistle::IntParameter *, sensei::IntOptions> m_intOptions;
    int m_instanceNum = 0;
    std::unique_ptr<std::thread> m_communicationThread;
    std::mutex m_communicationMutex;
    std::atomic_bool m_terminateCommunication{false};
    std::vector<vistle::message::Buffer> m_vistleObjects;
    //..........................................................................
    // module functions
    bool changeParameter(const Parameter *p) override;

    virtual bool prepare() override;
    void connectionAdded(const Port *from, const Port *to) override;
    void connectionRemoved(const Port *from, const Port *to) override;

    //..........................................................................
    void startCommunicationThread();
    void terminateCommunicationThread();

    void communicateWithSim();
    void initRecvFromSimQueue();
    vistle::insitu::message::ModuleInfo::ShmInfo gatherModuleInfo() const;
    //sync meta data with sim so that all objects that are (implicitly) sent by this module
    //share the same iteration and execution counter
    void updateMeta(const vistle::message::Buffer &obj);

    bool recvAndhandleMessage();
    bool recvVistleObjects();
    bool chacheVistleObjects();

    bool handleMessage(insitu::message::Message &msg);

    void connectToSim();
    void sendIntOptions();

    void disconnectSim();
    void addIntParam(vistle::IntParameter *param, sensei::IntOptions o);
};
} // namespace sensei
} // namespace insitu
} // namespace vistle

#endif // !SENSEI_CONTROL_MODULE_H
