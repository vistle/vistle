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
    mpi::communicator m_simulationCommandsComm;

    virtual std::unique_ptr<insitu::message::MessageHandler> connectToSim() = 0;
    virtual bool changeParameter(const Parameter *p) override;
    void initializeCommunication();

private:
    std::unique_ptr<insitu::message::MessageHandler> m_messageHandler;
    std::map<std::string, vistle::Port *> m_outputPorts; // output ports for the data the simulation offers
    std::set<vistle::Parameter *> m_commandParameter; // buttons to trigger simulation commands
    std::set<vistle::Parameter *> m_customCommandParameter; // string inputs to trigger simulation commands

    std::unique_ptr<vistle::message::MessageQueue>
        m_vistleObjectsMessageQueue; // receives vistle messages that will be passed through to manager

    //...................................................................................

    int m_instanceNum = 0; //to get a unique shm name when connecting to a different simulation
    std::atomic_bool m_terminateCommunication{false};
    //this thread receives and handles messages from the simulation (via m_messageHandler)
    std::unique_ptr<std::thread> m_simulationCommandsThread;
    //this thread caches the produced data objects for prepare to forward them to the vistle pipeline
    std::unique_ptr<std::thread> m_vistleObjectsThread;
    std::mutex m_vistleObjectsMutex;
    mpi::communicator m_vistleObjectsComm;
    std::vector<std::vector<vistle::message::Buffer>> m_cachedVistleObjects;


    //..........................................................................
    // module functions

    bool prepare() override;
    void connectionAdded(const Port *from, const Port *to) override;
    void connectionRemoved(const Port *from, const Port *to) override;

    //..........................................................................
    void startCommunicationThreads();
    void terminateCommunicationThreads();

    void communicateWithSim();
    void initRecvFromSimQueue();
    vistle::insitu::message::ModuleInfo::ShmInfo gatherModuleInfo() const;
    //sync this modules meta data with sim's data objects
    //this sim interface decides on when to update the iteration and execution counter
    void updateMeta(const vistle::message::Buffer &obj);

    bool recvAndhandleMessage();
    void recvVistleObjects();
    bool cacheVistleObjects();

    bool handleInsituMessage(insitu::message::Message &msg);

    //sent info about this modules int parameters (parameters in m_intOptions) to the sim
    void sendIntOptions();

    void disconnectSim();
};

} // namespace insitu
} // namespace vistle

#endif // INSITU_MODULE_H
