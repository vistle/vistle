#ifndef INSITU_MODULE_H
#define INSITU_MODULE_H

#include "export.h"
#include <vistle/core/messagequeue.h>
#include <vistle/module/module.h>

#include <string>
#include <vector>
#include <list>
#include <queue>

#include <vistle/insitu/message/MessageHandler.h>

namespace vistle {
namespace insitu {
//Base class for in situ modules (InSituModule for SENSEI and Catalyst II, LibSim and MiniSim)
class V_INSITUMODULEEXPORT InSituModuleBase: public vistle::Module {
public:
    InSituModuleBase(const std::string &name, const int moduleID, mpi::communicator comm);
    ~InSituModuleBase();

    bool isConnectedToSim() const;

protected:
    vistle::StringParameter *m_filePath =
        nullptr; //file with connection information -> triggers connectToSim() when changed
    std::vector<const vistle::IntParameter *>
        m_intOptions; //set of int parameters that are synchronized with the simulation
    mpi::communicator m_simulationCommandsComm; //mpi communicator for the simulation commands

    //use connection information from m_filePath to connect to the simulation
    //use m_simulationCommandsComm to synchronize the resutls from reading m_filePath
    virtual std::unique_ptr<insitu::message::MessageHandler> connectToSim() = 0;

    bool changeParameter(const Parameter *p) override;
    //! notify that a module has added a parameter
    bool parameterAdded(const int senderId, const std::string &name, const vistle::message::AddParameter &msg,
                        const std::string &moduleName) override;
    void initializeCommunication();
    const insitu::message::MessageHandler *getMessageHandler() const;

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
    std::queue<std::vector<vistle::message::Buffer>> m_cachedVistleObjects;


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
    //this sim interface decides on when to update the iteration and generation
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
