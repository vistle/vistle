#ifndef INSITU_READER_H
#define INSITU_READER_H

#include <module/module.h>
#include "export.h"
#include<core/messagequeue.h>
#include <insitu/message/SyncShmIDs.h>



namespace vistle{
namespace insitu {


//this type of module only calls prepare and reduce at the start/ end of the execution process.
//it handles input from the manager also during execution
//when execution starts (prepare) a Simulation that shares the shm area of this module must communicate it's shm ids via SyncShmIDs object
//the sim must only create shm objects while this module m_isExecuting while these module must create vistle objects only if !m_isExecuting
//input ports are not tested on the InSituReader
class V_INSITUMODULEEXPORT InSituReader : public vistle::Module {
public:
    InSituReader(const std::string& description, const std::string& name, const int moduleID, mpi::communicator comm);
    bool isExecuting();
    //use these function to make sure that the insitu process only creates vistle objects after beginExecute and before endExecute.
    virtual bool beginExecute() = 0;
    virtual bool endExecute() = 0;
    virtual bool operate();
    virtual void cancelExecuteMessageReceived(const vistle::message::Message* msg) override;
    size_t InstanceNum() const;
    void reconnect();

    virtual bool sendMessage(const vistle::message::Message& message, const buffer* payload = nullptr) const override;


  private:

    bool handleExecute(const vistle::message::Execute* exec) override final;
    bool dispatch(bool block = true, bool* messageReceived = nullptr) override final;
    bool prepare() override final;
    void initRecvFromSimQueue();

    bool m_isExecuting = false;
    const vistle::message::Execute* m_exec = nullptr;
    std::unique_ptr<vistle::message::MessageQueue> m_receiveFromSimMessageQueue; //receives vistle messages that will be passed through to manager
    std::string m_receiveFromSimMessageQueueName = "recvFromSimMsq";
    size_t m_instanceNum = 0;
    static size_t m_numInstances;

    insitu::message::SyncShmIDs m_shmIDs;
};

}//insitu
}//vistle



#endif