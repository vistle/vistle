#include <sstream>
#include <iomanip>

#include "message.h"
#include "messagequeue.h"

using namespace boost::interprocess;

namespace vistle {
namespace message {

std::string MessageQueue::createName(const char * prefix,
                                     const int moduleID, const int rank) {

   std::stringstream mqID;
   mqID << "vistle" << prefix << std::setw(8) << std::setfill('0') << moduleID
        << "_" << std::setw(8) << std::setfill('0') << rank;

   return mqID.str();
}

MessageQueue * MessageQueue::create(const std::string & n) {

   message_queue::remove(n.c_str());
   return new MessageQueue(n, boost::interprocess::create_only);
}

MessageQueue * MessageQueue::open(const std::string & n) {

   return new MessageQueue(n, boost::interprocess::open_only);
}

MessageQueue::MessageQueue(const std::string & n,
                           boost::interprocess::create_only_t)
   : name(n),
     mq(create_only, name.c_str(), 64 /* num msg */,
        message::Message::MESSAGE_SIZE),
     removeOnExit(true) {

}

MessageQueue::MessageQueue(const std::string & n,
                           boost::interprocess::open_only_t)
   : name(n), mq(open_only, name.c_str()), removeOnExit(false) {

}

MessageQueue::~MessageQueue() {

   if (removeOnExit)
      message_queue::remove(name.c_str());
}

const std::string & MessageQueue::getName() const {

   return name;
}

boost::interprocess::message_queue &  MessageQueue::getMessageQueue() {

   return mq;
}

} // namespace message
} // namespace vistle
