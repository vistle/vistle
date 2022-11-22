#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <deque>
#include <map>
#include <mutex>
#include <vistle/util/boost_interprocess_config.h>
#define BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX
#include <boost/interprocess/ipc/message_queue.hpp>

#include "message.h"
#include "export.h"

namespace vistle {
namespace message {

class Message;

class V_COREEXPORT MessageQueue {
public:
    typedef boost::interprocess::message_queue message_queue;

    static MessageQueue *create(const std::string &m_name);
    static MessageQueue *open(const std::string &m_name);

    static std::string createName(const char *prefix, const int moduleID, const int rank);

    void makeNonBlocking();

    ~MessageQueue();

    const std::string &getName() const;

    void signal(); // immediately send zero-size message
    bool send(const Message &msg, unsigned int priority = 0);
    bool progress();

    bool receive(Message &msg, unsigned int *priority = nullptr);
    bool tryReceive(Message &msg, unsigned int minPrio = 0, unsigned int *priority = nullptr);
    size_t getNumMessages();

private:
    bool m_blocking = true;
    MessageQueue(const std::string &m_name, boost::interprocess::create_only_t);
    MessageQueue(const std::string &m_name, boost::interprocess::open_only_t);

    const std::string m_name;
    message_queue m_mq;
    std::deque<message::Buffer> m_queue; // for messages with prioritiy 0
    std::map<unsigned int, std::deque<message::Buffer>> m_prioQueues; // for messages with higher priority
    std::mutex m_mutex;
};

} // namespace message
} // namespace vistle
#endif
