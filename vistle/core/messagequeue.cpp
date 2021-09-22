#include <sstream>
#include <fstream>
#include <iomanip>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread_time.hpp>

#include <vistle/util/tools.h>
#include "message.h"
#include "messagequeue.h"
#include "shm.h"
#include <cassert>

#ifdef __linux__
// not necessary, as child processes die with their parent
#define NO_CHECK_FOR_DEAD_PARENT
#endif

using namespace boost::interprocess;

namespace vistle {
namespace message {

std::string MessageQueue::createName(const char *prefix, const int moduleID, const int rank)
{
    std::stringstream mqID;
    mqID << Shm::the().instanceName() << "_" << prefix << "_m" << moduleID << "_r" << rank;

    return mqID.str();
}

MessageQueue *MessageQueue::create(const std::string &n)
{
    {
        std::ofstream f;
        f.open(Shm::shmIdFilename().c_str(), std::ios::app);
        f << n << std::endl;
    }

    message_queue::remove(n.c_str());
    return new MessageQueue(n, create_only);
}

MessageQueue *MessageQueue::open(const std::string &n)
{
    auto ret = new MessageQueue(n, open_only);
    message_queue::remove(n.c_str());
    //std::cerr << "MessageQueue: opened and removed " << n << std::endl;
    return ret;
}

MessageQueue::MessageQueue(const std::string &n, create_only_t)
: m_blocking(true), m_name(n), m_mq(create_only, m_name.c_str(), 10 /* num msg */, message::Message::MESSAGE_SIZE)
{}

MessageQueue::MessageQueue(const std::string &n, open_only_t)
: m_blocking(true), m_name(n), m_mq(open_only, m_name.c_str())
{}

MessageQueue::~MessageQueue()
{
    message_queue::remove(m_name.c_str());
}

void MessageQueue::makeNonBlocking()
{
    m_blocking = false;
}

const std::string &MessageQueue::getName() const
{
    return m_name;
}

bool MessageQueue::progress()
{
    std::unique_lock<std::mutex> guard(m_mutex);
    while (!m_queue.empty()) {
        guard.unlock();
        if (m_blocking) {
            guard.lock();
            if (!m_queue.empty()) {
                m_mq.send(m_queue.front().data(), message::Message::MESSAGE_SIZE, 0);
                m_queue.pop_front();
            }
        } else {
            guard.lock();
            if (!m_queue.empty()) {
                if (!m_mq.try_send(m_queue.front().data(), message::Message::MESSAGE_SIZE, 0)) {
                    return m_queue.empty();
                }
                m_queue.pop_front();
            }
        }
    }
    return m_queue.empty();
}

bool MessageQueue::send(const Message &msg)
{
    std::unique_lock<std::mutex> guard(m_mutex);
    m_queue.emplace_back(msg);
    guard.unlock();
    return progress();
}

void MessageQueue::receive(Message &msg)
{
    size_t recvSize = 0;
    unsigned priority = 0;
#ifdef NO_CHECK_FOR_DEAD_PARENT
    m_mq.receive(&msg, message::Message::MESSAGE_SIZE, recvSize, priority);
#else
    while (!m_mq.timed_receive(&msg, message::Message::MESSAGE_SIZE, recvSize, priority,
                               boost::get_system_time() + boost::posix_time::seconds(5))) {
        if (parentProcessDied())
            throw except::parent_died();
    }
#endif
    assert(recvSize == message::Message::MESSAGE_SIZE);
}

bool MessageQueue::tryReceive(Message &msg)
{
#ifndef NO_CHECK_FOR_DEAD_PARENT
    if (parentProcessDied())
        throw except::parent_died();
#endif

    size_t recvSize = 0;
    unsigned priority = 0;
    bool result = m_mq.try_receive(&msg, message::Message::MESSAGE_SIZE, recvSize, priority);
    if (result) {
        assert(recvSize == message::Message::MESSAGE_SIZE);
    }
    return result;
}

size_t MessageQueue::getNumMessages()
{
    return m_mq.get_num_msg();
}

} // namespace message
} // namespace vistle
