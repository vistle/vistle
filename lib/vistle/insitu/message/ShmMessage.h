#ifndef INSITU_SHM_MESSAGE_H
#define INSITU_SHM_MESSAGE_H
#include "InSituMessage.h"
#include "MessageHandler.h"
#include "export.h"
#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <mpi.h>
namespace vistle {
namespace insitu {
namespace message {
constexpr size_t ShmMessageMaxSize = 1000;
constexpr unsigned int ShmMessageQueueLenght = 20;
struct ShmMsg {
    int type = 0;
    size_t size = 0;
    std::array<char, ShmMessageMaxSize> buf;
};

class V_INSITUMESSAGEEXPORT InSituShmMessage: public MessageHandler {
public:
    InSituShmMessage(MPI_Comm Comm); // create a msq
    InSituShmMessage(const std::string &msqName, MPI_Comm Comm); // connect to a msq
    ~InSituShmMessage();
    bool sendMessage(InSituMessageType type, const vistle::buffer &msg) const override;

    bool isInitialized();
    void removeShm();
    std::string name();
    insitu::message::Message recv() override;
    insitu::message::Message tryRecv() override;
    insitu::message::Message timedRecv(size_t timeInSec);

private:
    std::array<std::unique_ptr<boost::interprocess::message_queue>, 2> m_msqs;
    bool m_creator = false; // if true we created shm objects which we have to remove
    const std::string m_msqName = "vistle_shmMessage_";
    const std::string m_recvSuffix =
        "_recv_sensei"; // signature must contain _recv_ for shm::cleanAll to remove these files
    const std::string m_sendSuffix =
        "_send_sensei"; // signature must contain _send_ for shm::cleanAll to remove these files
    size_t m_iteration = 0;
    MPI_Comm m_comm;
    int m_rank;
};

} // namespace message
} // namespace insitu
} // namespace vistle

#endif // !INSITU_SHM_MESSAGE_H
