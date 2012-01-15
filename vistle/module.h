#ifndef MODULE_H
#define MODULE_H

namespace vistle {

class MessageQueue;

class Module {

 public:
   Module(int rank, int size, int moduleID,
          MessageQueue &rmq, MessageQueue &smq);

   ~Module();

   bool dispatch();

 private:
   bool handleMessage(message::Message *message);

   const int rank;
   const int size;
   const int moduleID;

   MessageQueue & receiveMessageQueue;
   MessageQueue & sendMessageQueue;
};

} // namespace vistle

#endif
