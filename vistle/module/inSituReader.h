#ifndef INSITU_READER_H
#define INSITU_READER_H

#include "module.h"
#include "export.h"
namespace vistle {


//this type of module only calls prepare and reduce at the start/ end of the execution process.
//it handles input from the manager also during execution
//input ports are not tested on the InSituReader
class V_MODULEEXPORT InSituReader : public Module {
public:
    InSituReader(const std::string& description, const std::string& name, const int moduleID, mpi::communicator comm);
    ~InSituReader();
    bool isExecuting();

  private:
      //aditionally forwards messages from the simulation to the manager
    virtual bool dispatch(bool block = true, bool* messageReceived = nullptr) override;
    virtual bool handleExecute(const message::Execute* exec) override;
    virtual void cancelExecuteMessageReceived(const message::Message* msg) override;



    bool m_isExecuting = false;
    const vistle::message::Execute* m_exec;
    message::MessageQueue* m_receiveFromSimMessageQueue = nullptr;
};


}



#endif