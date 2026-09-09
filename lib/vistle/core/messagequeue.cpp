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
#else
#ifdef MODULE_THREAD
// checking in every thread is overkill
#define NO_CHECK_FOR_DEAD_PARENT
#endif
#endif

namespace interprocess = boost::interprocess;

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
    return new MessageQueue(n, interprocess::create_only);
}

MessageQueue *MessageQueue::open(const std::string &n)
{
    auto ret = new MessageQueue(n, interprocess::open_only);
    message_queue::remove(n.c_str());
    //std::cerr << "MessageQueue: opened and removed " << n << std::endl;
    return ret;
}

MessageQueue::MessageQueue(const std::string &n, interprocess::create_only_t)
: m_blocking(true)
, m_name(n)
, m_mq(interprocess::create_only, m_name.c_str(), 10 /* num msg */, message::Message::MESSAGE_SIZE)
{}

MessageQueue::MessageQueue(const std::string &n, interprocess::open_only_t)
: m_blocking(true), m_name(n), m_mq(interprocess::open_only, m_name.c_str())
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
    guard.unlock();

    auto process_queue = [this, &guard](std::deque<message::Buffer> &queue) -> bool {
        while (!queue.empty()) {
            guard.unlock();
            if (m_blocking) {
                guard.lock();
                if (!queue.empty()) {
                    m_mq.send(queue.front().data(), message::Message::MESSAGE_SIZE, 0);
                    queue.pop_front();
                }
            } else {
                guard.lock();
                if (!queue.empty()) {
                    if (!m_mq.try_send(queue.front().data(), message::Message::MESSAGE_SIZE, 0)) {
                        break;
                    }
                    queue.pop_front();
                }
            }
        }
        return queue.empty();
    };

    guard.lock();
    for (auto it = m_prioQueues.begin(), next = it; it != m_prioQueues.end(); it = next) {
        ++next;
        auto &q = it->second;
        bool empty = process_queue(q);
        if (empty) {
            next = m_prioQueues.erase(it);
        } else if (!m_blocking) {
            return false;
        }
    }
    return process_queue(m_queue);
}

void MessageQueue::signal()
{
    std::unique_lock<std::mutex> guard(m_mutex);
    m_mq.send(nullptr, 0, 0);
}

bool MessageQueue::send(const ShmEnvelope &envelope, unsigned int priority)
{
    assert(envelope.externalPayloadSize() == 0 || envelope.shmPayload());
    std::unique_lock<std::mutex> guard(m_mutex);
    envelope.ref();
    if (priority == 0) {
        m_queue.emplace_back(envelope.message());
    } else {
        m_prioQueues[priority].emplace_back(envelope.message());
    }
    guard.unlock();
    return progress();
}

bool MessageQueue::receive(ShmEnvelope &envelope, unsigned int *ppriority)
{
    size_t recvSize = 0;
    unsigned priority = 0;
#ifdef NO_CHECK_FOR_DEAD_PARENT
    m_mq.receive(&envelope.message(), message::Message::MESSAGE_SIZE, recvSize, priority);
#else
    while (!m_mq.timed_receive(&envelope.message(), message::Message::MESSAGE_SIZE, recvSize, priority,
                               boost::get_system_time() + boost::posix_time::seconds(5))) {
        if (parentProcessDied())
            throw except::parent_died();
    }
#endif
    if (ppriority)
        *ppriority = priority;
    if (recvSize != message::Message::MESSAGE_SIZE)
        return false;
    if (envelope.message().payloadSize() > 0) {
        envelope.updateMessage();
        if (!envelope.hasInternalPayload()) {
            envelope.constructPayloadfromShm();
            envelope.unref(); // sender increased it so that it is not deleted until received
        }
    }
    return true;
}

bool MessageQueue::tryReceive(ShmEnvelope &envelope, unsigned int minPrio, unsigned int *ppriority)
{
#ifndef NO_CHECK_FOR_DEAD_PARENT
    if (parentProcessDied())
        throw except::parent_died();
#endif

    size_t recvSize = 0;
    unsigned priority = 0;
    bool result = m_mq.try_receive(&envelope.message(), message::Message::MESSAGE_SIZE, recvSize, priority);
    if (result) {
        result = recvSize == message::Message::MESSAGE_SIZE;
        if (envelope.message().payloadSize() > 0) {
            envelope.updateMessage();
            if (!envelope.hasInternalPayload()) {
                envelope.constructPayloadfromShm();
                envelope.unref(); // sender increased it so that it is not deleted until received
            }
        }
    }
    if (ppriority)
        *ppriority = priority;
    return result;
}

size_t MessageQueue::getNumMessages()
{
    return m_mq.get_num_msg();
}

} // namespace message
} // namespace vistle
