#include "ShmMessage.h"
#include <fstream>
#include <vistle/insitu/core/exception.h>
using namespace vistle::insitu::message;
bool InSituShmMessage::sendMessage(InSituMessageType type, const vistle::buffer &vec) const
{ // not thread safe

    std::vector<ShmMsg> msgs;
    int i = 0;
    auto start = vec.begin();
    auto end = vec.begin();
    while (end != vec.end()) {
        auto &mm = msgs.emplace_back(ShmMsg{static_cast<int>(type), vec.size()});
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

InSituShmMessage::InSituShmMessage(int rank)
{
    m_creator = true;
    bool error = false;
    std::string msqName;
    do {
        error = false;
        msqName = m_msqName + std::to_string(++m_iteration) + "_r" + std::to_string(rank);
        try {
            std::cerr << "creating msq " << (msqName + m_recvSuffix).c_str() << std::endl;
            m_msqs[0] = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::create_only,
                                                                             (msqName + m_recvSuffix).c_str(),
                                                                             ShmMessageQueueLenght, sizeof(ShmMsg));
            std::cerr << "creating msq " << (msqName + m_sendSuffix).c_str() << std::endl;
            m_msqs[1] = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::create_only,
                                                                             (msqName + m_sendSuffix).c_str(),
                                                                             ShmMessageQueueLenght, sizeof(ShmMsg));
            {
                std::ofstream f;
                f.open(Shm::shmIdFilename().c_str(), std::ios::app);
                f << msqName + m_recvSuffix << std::endl;
                f << msqName + m_sendSuffix << std::endl;
            }
        } catch (const boost::interprocess::interprocess_exception &ex) {
            std::cerr << "ShmMessage " << msqName << " creation error: " << ex.what() << std::endl;
            error = true;
        }
    } while (error);
    std::cerr << "ShmMessage " << msqName << " creation successful!" << std::endl;
}

InSituShmMessage::InSituShmMessage(const std::string &msqName, int rank)
{
    auto msqRankName = msqName + "_r" + std::to_string(rank);
    try {
        std::cerr << "trying to open " << (msqRankName + m_sendSuffix).c_str() << std::endl;
        m_msqs[0] = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::open_only,
                                                                         (msqRankName + m_sendSuffix).c_str());
        std::cerr << "trying to open " << (msqRankName + m_recvSuffix).c_str() << std::endl;
        m_msqs[1] = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::open_only,
                                                                         (msqRankName + m_recvSuffix).c_str());

        boost::interprocess::message_queue::remove((msqRankName + m_sendSuffix).c_str());
        boost::interprocess::message_queue::remove((msqRankName + m_recvSuffix).c_str());
    } catch (const boost::interprocess::interprocess_exception &ex) {
        throw InsituException{} << "ShmMessage " << msqRankName << " opening error: " << ex.what();
    }
}

void vistle::insitu::message::InSituShmMessage::removeShm()
{
    for (size_t i = 0; i < 100; i++) {
        std::string msqName = m_msqName + std::to_string(i);
        boost::interprocess::shared_memory_object::remove((msqName + m_recvSuffix).c_str());
        boost::interprocess::shared_memory_object::remove((msqName + m_sendSuffix).c_str());
    }
}

std::string InSituShmMessage::name()
{
    if (m_creator) {
        return m_msqName + std::to_string(m_iteration);
    }
    return m_msqName;
}

Message InSituShmMessage::recv()
{
    vistle::buffer payload;
    int type = 0;
    vistle::buffer::iterator iter;

    ShmMsg m;
    size_t recvSize = 0;
    unsigned int prio = 0;

    size_t left = ShmMessageMaxSize;
    size_t i = 0;
    while (left > 0) {
        try {
            m_msqs[0]->receive((void *)&m, sizeof(m), recvSize, prio);
        } catch (const boost::interprocess::interprocess_exception &ex) {
            std::cerr << "ShmMessage recv error: " << ex.what() << std::endl;
            return Message::errorMessage();
        }
        if (i == 0) {
            payload.resize(m.size);
            iter = payload.begin();
            type = m.type;
            left = m.size;
        }
        assert(type == m.type);
        std::copy(m.buf.begin(), m.buf.begin() + std::min(left, ShmMessageMaxSize), iter + i * ShmMessageMaxSize);
        left = left > ShmMessageMaxSize ? left - ShmMessageMaxSize : 0;
        ++i;
    }
    return Message{static_cast<InSituMessageType>(type), std::move(payload)};
}

Message InSituShmMessage::tryRecv()
{
    vistle::buffer payload;
    int type = 0;
    auto iter = payload.begin();
    unsigned int i = 1;

    ShmMsg m;
    size_t recvSize = 0;
    unsigned int prio = 0;
    size_t left = 0;
    try {
        if (!m_msqs[0]->try_receive((void *)&m, sizeof(m), recvSize, prio)) {
            return Message::errorMessage();
        }
        payload.resize(m.size);
        type = m.type;
        iter = payload.begin();
        left = m.size;

        std::copy(m.buf.begin(), m.buf.begin() + std::min(left, ShmMessageMaxSize), iter);
        left = left > ShmMessageMaxSize ? left - ShmMessageMaxSize : 0;
    } catch (const boost::interprocess::interprocess_exception &ex) {
        std::cerr << "ShmMessage tryReceive error: " << ex.what() << std::endl;
        return Message::errorMessage();
    }
    while (left > 0) {
        try {
            m_msqs[0]->receive((void *)&m, sizeof(m), recvSize, prio);
        } catch (const boost::interprocess::interprocess_exception &ex) {
            std::cerr << "ShmMessage tryRecv error: " << ex.what() << std::endl;
            return Message::errorMessage();
        }
        assert(type == m.type);
        std::copy(m.buf.begin(), m.buf.begin() + std::min(left, ShmMessageMaxSize), iter + i * ShmMessageMaxSize);
        left = left > ShmMessageMaxSize ? left - ShmMessageMaxSize : 0;
        ++i;
    }

    return Message{static_cast<InSituMessageType>(type), std::move(payload)};
}

Message InSituShmMessage::timedRecv(size_t timeInSec)
{
    vistle::buffer payload;
    int type = 0;
    vistle::buffer::iterator iter;

    ShmMsg m;
    size_t recvSize = 0;
    unsigned int prio = 0;
    auto now = boost::posix_time::microsec_clock::universal_time();
    auto endTime = now + boost::posix_time::seconds(timeInSec);

    size_t left = ShmMessageMaxSize;
    size_t i = 0;
    while (left > 0) {
        try {
            if (!m_msqs[0]->timed_receive((void *)&m, sizeof(m), recvSize, prio, endTime)) {
                return Message::errorMessage();
            }
        } catch (const boost::interprocess::interprocess_exception &ex) {
            std::cerr << "ShmMessage timedRecv error: " << ex.what() << std::endl;
            return Message::errorMessage();
        }
        if (i == 0) {
            payload.resize(m.size);
            iter = payload.begin();
            type = m.type;
            left = m.size;
        }
        assert(type == m.type);
        std::copy(m.buf.begin(), m.buf.begin() + std::min(left, ShmMessageMaxSize), iter + i * ShmMessageMaxSize);
        left = left > ShmMessageMaxSize ? left - ShmMessageMaxSize : 0;
        ++i;
    }
    return Message{static_cast<InSituMessageType>(type), std::move(payload)};
}

vistle::insitu::message::InSituShmMessage::~InSituShmMessage()
{
    if (!m_creator) {
        return;
    }
    std::string msqName = m_msqName + std::to_string(m_iteration);
    boost::interprocess::shared_memory_object::remove((msqName + m_recvSuffix).c_str());
    boost::interprocess::shared_memory_object::remove((msqName + m_sendSuffix).c_str());
}
