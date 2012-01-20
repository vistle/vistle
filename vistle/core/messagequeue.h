#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <boost/interprocess/ipc/message_queue.hpp>

namespace vistle {
namespace message {

class MessageQueue {

 public:
   static MessageQueue * create(const std::string & name);
   static MessageQueue * open(const std::string & name);

   static std::string createName(const char * prefix,
                                 const int moduleID, const int rank);

   ~MessageQueue();

   const std::string & getName() const;
   boost::interprocess::message_queue & getMessageQueue();

 private:
   MessageQueue(const std::string & name, boost::interprocess::create_only_t);
   MessageQueue(const std::string & name, boost::interprocess::open_only_t);

   const std::string name;
   boost::interprocess::message_queue mq;
   bool removeOnExit;
};

} // namespace message
} // namespace vistle
#endif
