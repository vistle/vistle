#ifndef SENSEI_CONTROL_MODULE_H
#define SENSEI_CONTROL_MODULE_H


#include <vistle/core/message.h>
#include <vistle/core/messagequeue.h>
#include <vistle/insitu/message/InSituMessage.h>
#include <vistle/insitu/message/ShmMessage.h>
#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/insitu/module/inSituModule.h>
#include <vistle/module/module.h>

#include <thread>
#include <chrono>

namespace vistle {
namespace insitu {
namespace message {
class InSituShmMessage;
}
namespace sensei {

class SenseiModule: public vistle::insitu::InSituModule {
public:
    typedef std::lock_guard<std::mutex> Guard;

    SenseiModule(const std::string &name, int moduleID, mpi::communicator comm);

private:
    std::unique_ptr<insitu::message::MessageHandler> connectToSim() override;
};
} // namespace sensei
} // namespace insitu
} // namespace vistle

#endif // !SENSEI_CONTROL_MODULE_H
