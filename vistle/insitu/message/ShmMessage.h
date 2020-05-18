#ifndef INSITU_SHM_MESSAGE_H
#define INSITU_SHM_MESSAGE_H
#include "InSituMessage.h"
#include "export.h"
#include <boost/interprocess/ipc/message_queue.hpp>
namespace vistle {
namespace insitu {
namespace message {
constexpr unsigned int ShmMessageMaxSize = 1000;
constexpr unsigned int ShmMessageQueueLenght = 20;
struct ShmMsg {
    int type = 0;
    size_t size = 0;
    char buf[ShmMessageMaxSize];
};

class V_INSITUMESSAGEEXPORT InSituShmMessage {
public:
    template<typename SomeMessage>
    bool send(const SomeMessage& msg) const { //not thread safe
        if (!m_initialized) {
            std::cerr << "ShmMessage uninitialized: can not send message!" << std::endl;
            return false;
        }
        if (msg.type == InSituMessageType::Invalid) {
            std::cerr << "ShmMessage : can not send invalid message!" << std::endl;
            return false;
        }
        vistle::vecostreambuf<vistle::buffer> buf;
        vistle::oarchive ar(buf);
        ar& msg;
        vistle::buffer vec = buf.get_vector();
        std::vector<ShmMsg> msgs;
        int i = 0;
        auto start = vec.begin();
        auto end = vec.begin();
        while (end != vec.end())
        {
            auto mm = msgs.emplace_back(ShmMsg{ static_cast<int>(msg.type), vec.size() });
            start += i;
            if (i += ShmMessageMaxSize < vec.size())
            {
                end += ShmMessageMaxSize;
            }
            else {
                end = vec.end();
            }
            std::copy(start, end, mm.buf);
        }

        try
        {
            for (auto msg : msgs)
            {
                m_msqs[m_order[1]]->send((void*)&msg, sizeof(msg), 0);
            }
        }
        catch (const boost::interprocess::interprocess_exception& ex)
        {
            std::cerr << "ShmMessage failed to send message: " << ex.what() << std::endl;
            return false;
        }

        return true;
    }
    void initialize();
    void initialize(const std::string& msqName);
    bool isInitialized();
    std::string name();
    insitu::message::Message recv();
    insitu::message::Message tryRecv();
    insitu::message::Message timedRecv(size_t timeInSec);
private:
    bool m_initialized = false;
    std::array<std::unique_ptr<boost::interprocess::message_queue>, 2> m_msqs;
    std::array<int, 2> m_order; //0: recv queu 1: send queue
    std::string m_msqName = "vislte_sensei_shmMessage_";

};


} //message
} //insitu
} //vistle

#endif // !INSITU_SHM_MESSAGE_H
