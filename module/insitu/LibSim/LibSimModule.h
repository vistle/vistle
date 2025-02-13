#ifndef VISTLE_LIBSIM_LIBSIMMODULE_H
#define VISTLE_LIBSIM_LIBSIMMODULE_H

#include <vistle/core/message.h>
#include <vistle/core/messagequeue.h>
#include <vistle/insitu/message/MessageHandler.h>
#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/insitu/message/TcpMessage.h>
#include <vistle/insitu/module/inSituModuleBase.h>
#include <vistle/module/module.h>

class LibSimModule: public vistle::insitu::InSituModuleBase {
public:
    LibSimModule(const std::string &name, int moduleID, mpi::communicator comm);
    ~LibSimModule();

private:
    std::unique_ptr<vistle::insitu::message::MessageHandler> connectToSim() override;
    void disconnectSim();


    //..........................................................................


#ifndef MODULE_THREAD
    vistle::StringParameter *m_simName = nullptr;
#endif
};

#endif
