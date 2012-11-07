#include <sstream>
#include <iomanip>

#include "message.h"
#include "messagequeue.h"
#include "shm.h"

using namespace boost::interprocess;

namespace vistle {
namespace message {

std::string MessageQueue::createName(const char * prefix,
                                     const int moduleID, const int rank) {

   std::stringstream mqID;
   mqID << "mq_" << Shm::the().getName() << "_" << prefix << "_" << std::setw(8) << std::setfill('0') << moduleID
        << "_" << std::setw(8) << std::setfill('0') << rank;

   return mqID.str();
}

MessageQueue * MessageQueue::create(const std::string & n) {

   message_queue::remove(n.c_str());
   return new MessageQueue(n, create_only);
}

MessageQueue * MessageQueue::open(const std::string & n) {

   {
      std::ofstream f;
      f.open(Shm::shmIdFilename().c_str(), std::ios::app);
      f << n << std::endl;
   }

   return new MessageQueue(n, open_only);
}

MessageQueue::MessageQueue(const std::string & n, create_only_t)
   : name(n),  mq(create_only, name.c_str(), 256 /* num msg */,
                  message::Message::MESSAGE_SIZE),
     removeOnExit(true) {

}

MessageQueue::MessageQueue(const std::string & n, open_only_t)
   : name(n), mq(open_only, name.c_str()), removeOnExit(false) {

}

MessageQueue::~MessageQueue() {

   if (removeOnExit)
      message_queue::remove(name.c_str());
}

const std::string & MessageQueue::getName() const {

   return name;
}

message_queue &  MessageQueue::getMessageQueue() {

   return mq;
}

} // namespace message
} // namespace vistle
