#ifndef INSITU_CONTROL_MODULE_H
#define INSITU_CONTROL_MODULE_H


#include <vistle/core/message.h>
#include <vistle/core/messagequeue.h>
#include <vistle/insitu/message/InSituMessage.h>
#include <vistle/insitu/message/ShmMessage.h>
#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/insitu/module/inSituModuleBase.h>
#include <vistle/module/module.h>

#include <thread>
#include <chrono>

namespace vistle {
namespace insitu {
namespace message {
class InSituShmMessage;
}
class InSituModule: public vistle::insitu::InSituModuleBase {
public:
    typedef std::lock_guard<std::mutex> Guard;

    InSituModule(const std::string &name, int moduleID, mpi::communicator comm);

private:
    std::unique_ptr<insitu::message::MessageHandler> connectToSim() override;
};
} // namespace insitu
} // namespace vistle

#endif // !INSITU_CONTROL_MODULE_H
