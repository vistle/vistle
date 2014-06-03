#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <boost/interprocess/ipc/message_queue.hpp>

#include "export.h"

namespace vistle {
namespace message {

class Message;

class V_COREEXPORT MessageQueue {

 public:
   typedef boost::interprocess::message_queue message_queue;

   static MessageQueue * create(const std::string & m_name);
   static MessageQueue * open(const std::string & m_name);

   static std::string createName(const char * prefix,
                                 const int moduleID, const int rank);

   ~MessageQueue();

   const std::string & getName() const;

   void send(const Message &msg);
   void receive(Message &msg);
   bool tryReceive(Message &msg);
   size_t getNumMessages();

 private:
   MessageQueue(const std::string & m_name, boost::interprocess::create_only_t);
   MessageQueue(const std::string & m_name, boost::interprocess::open_only_t);

   const std::string m_name;
   message_queue m_mq;
};

} // namespace message
} // namespace vistle
#endif
