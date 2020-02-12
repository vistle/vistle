#ifndef INSITU_READER_H
#define INSITU_READER_H

#include <module/module.h>
#include "export.h"
namespace insitu {


//this type of module only calls prepare and reduce at the start/ end of the execution process.
//it handles input from the manager also during execution
//when execution starts (prepare) a Simulation that shares the shm area of this module must be informed about the shm ids via vistle::message::SYNCSHMIDS
//the sim can then create shm objects while this isExecuting
//input ports are not tested on the InSituReader
class V_INSITUMODULEEXPORT InSituReader : public vistle::Module {
public:
    InSituReader(const std::string& description, const std::string& name, const int moduleID, mpi::communicator comm);
    bool isExecuting();
    //use this function to get conformation that the external proces will not create vistle objects
    virtual bool prepareReduce() = 0;
  private:
    virtual bool handleExecute(const vistle::message::Execute* exec) override;
    virtual void cancelExecuteMessageReceived(const vistle::message::Message* msg) override;



    bool m_isExecuting = false;
    const vistle::message::Execute* m_exec;
};


}



#endif