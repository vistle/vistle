#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <deque>
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

    bool send(const Message &msg);
    bool progress();

    void receive(Message &msg);
    bool tryReceive(Message &msg);
    size_t getNumMessages();

private:
    bool m_blocking;
    MessageQueue(const std::string &m_name, boost::interprocess::create_only_t);
    MessageQueue(const std::string &m_name, boost::interprocess::open_only_t);

    const std::string m_name;
    message_queue m_mq;
    std::deque<message::Buffer> m_queue;
    std::mutex m_mutex;
};

} // namespace message
} // namespace vistle
#endif
