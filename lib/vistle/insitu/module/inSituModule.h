#ifndef INSITU_MODULE_H
#define INSITU_MODULE_H

#include "export.h"
#include <vistle/core/messagequeue.h>
#include <vistle/module/module.h>

#include <string>
#include <vector>
#include <list>

#include <vistle/insitu/message/MessageHandler.h>

namespace vistle {
namespace insitu {

class V_INSITUMODULEEXPORT InSituModule: public vistle::Module {
public:
    InSituModule(const std::string &name, const int moduleID, mpi::communicator comm);
    ~InSituModule();

    bool isConnectedToSim() const;

protected:
    vistle::StringParameter *m_filePath = nullptr;
    std::vector<const vistle::IntParameter *> m_intOptions;

    virtual std::unique_ptr<insitu::message::MessageHandler> connectToSim() = 0;
    virtual bool changeParameter(const Parameter *p) override;

private:
    std::unique_ptr<insitu::message::MessageHandler> m_messageHandler;
    std::map<std::string, vistle::Port *> m_outputPorts; // output ports for the data the simulation offers
    std::set<vistle::Parameter *> m_commandParameter; // buttons to trigger simulation commands
    std::set<vistle::Parameter *> m_customCommandParameter; // string inputs to trigger simulation commands

    std::unique_ptr<vistle::message::MessageQueue>
        m_receiveFromSimMessageQueue; // receives vistle messages that will be passed through to manager
    size_t m_ownExecutionCounter = 0;

    //...................................................................................

    int m_instanceNum = 0; //to get a unique shm name when connecting to a different simulation
    //this thread receives and handles messages from the simulation (via m_messageHandler) and caches the produces data objects
    //for prepare to forward them to the vistle pipeline
    std::unique_ptr<std::thread> m_communicationThread;
    std::mutex m_communicationMutex;
    std::atomic_bool m_terminateCommunication{false};
    std::list<vistle::message::Buffer> m_cachedVistleObjects;
    //number of timesteps/iterations tat have been received completely nad are not yet forwarded to the pipeline
    size_t m_numPackeges = 0;
    //..........................................................................
    // module functions

    bool prepare() override;
    void connectionAdded(const Port *from, const Port *to) override;
    void connectionRemoved(const Port *from, const Port *to) override;

    //..........................................................................
    void startCommunicationThread();
    void terminateCommunicationThread();

    void communicateWithSim();
    void initRecvFromSimQueue();
    vistle::insitu::message::ModuleInfo::ShmInfo gatherModuleInfo() const;
    //sync this modules meta data with sim's data objects
    //this sim interface decides on when to update the iteration and execution counter
    void updateMeta(const vistle::message::Buffer &obj);

    bool recvAndhandleMessage();
    bool recvVistleObjects();
    bool cacheVistleObjects();

    bool handleInsituMessage(insitu::message::Message &msg);

    //sent info about this modules int parameters (parameters in m_intOptions) to the sim
    void sendIntOptions();

    void disconnectSim();
};

} // namespace insitu
} // namespace vistle

#endif // INSITU_MODULE_H
