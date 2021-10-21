#ifndef INSITU_SHM_MESSAGE_H
#define INSITU_SHM_MESSAGE_H
#include "InSituMessage.h"
#include "export.h"
#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/ipc/message_queue.hpp>
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

class V_INSITUMESSAGEEXPORT InSituShmMessage {
public:
    template<typename SomeMessage>
    bool send(const SomeMessage &msg) const
    { // not thread safe
        if (!m_initialized) {
            std::cerr << "ShmMessage uninitialized: can not send message!" << std::endl;
            return false;
        }
        if (msg.type == InSituMessageType::Invalid) {
            std::cerr << "ShmMessage : can not send invalid message!" << std::endl;
            return false;
        }
        // std::cerr << "sending message of type " << static_cast<int>(msg.type) << std::endl;
        vistle::vecostreambuf<vistle::buffer> buf;
        vistle::oarchive ar(buf);
        ar &msg;
        vistle::buffer vec = buf.get_vector();
        std::vector<ShmMsg> msgs;
        int i = 0;
        auto start = vec.begin();
        auto end = vec.begin();
        while (end != vec.end()) {
            msgs.emplace_back(ShmMsg{static_cast<int>(msg.type), vec.size()});
            auto &mm = msgs[msgs.size() - 1];
            start += i;
            if (i += ShmMessageMaxSize < vec.size()) {
                end += ShmMessageMaxSize;
            } else {
                end = vec.end();
            }
            std::copy(start, end, mm.buf.begin());
        }

        try {
            for (auto msg: msgs) {
                m_msqs[1]->send((void *)&msg, sizeof(msg), 0);
            }
        } catch (const boost::interprocess::interprocess_exception &ex) {
            std::cerr << "ShmMessage failed to send message: " << ex.what() << std::endl;
            return false;
        }

        return true;
    }
    void initialize(int rank); // create a msq
    void initialize(const std::string &msqName, int rank); // connect to a msq
    void reset(); // set the state back to uninitialized
    bool isInitialized();
    void removeShm();
    std::string name();
    insitu::message::Message recv();
    insitu::message::Message tryRecv();
    insitu::message::Message timedRecv(size_t timeInSec);
    ~InSituShmMessage();

private:
    bool m_initialized = false;
    std::array<std::unique_ptr<boost::interprocess::message_queue>, 2> m_msqs;
    bool m_creator = false; // if true we created shm objects which we have to remove
    const std::string m_msqName = "vistle_shmMessage_";
    const std::string m_recvSuffix =
        "_recv_sensei"; // signature must contain _recv_ for shm::cleanAll to remove these files
    const std::string m_sendSuffix =
        "_send_sensei"; // signature must contain _send_ for shm::cleanAll to remove these files
    size_t m_iteration = 0;
};

} // namespace message
} // namespace insitu
} // namespace vistle

#endif // !INSITU_SHM_MESSAGE_H
