#include "message.h"
#include "messagequeue.h"

#include "renderer.h"

namespace vistle {

Renderer::Renderer(const std::string & name, const std::string &shmname,
                   const int rank, const int size, const int moduleID)
   : Module(name, shmname, rank, size, moduleID) {

   createInputPort("data_in", "input data");
}

Renderer::~Renderer() {

}

bool Renderer::dispatch() {

   size_t msgSize;
   unsigned int priority;
   char msgRecvBuf[message::Message::MESSAGE_SIZE];

   bool again = true;
   bool msg =
      receiveMessageQueue->getMessageQueue().try_receive(
                                               (void *) msgRecvBuf,
                                               message::Message::MESSAGE_SIZE,
                                               msgSize, priority);

   if (msg) {
      vistle::message::Message *message =
         (vistle::message::Message *) msgRecvBuf;
      again = handleMessage(message);

      if (!again) {
         vistle::message::ModuleExit m;
         sendMessageQueue->getMessageQueue().send(&m, sizeof(m), 0);
      }
   }

   if (again)
      render();

   return again;
}

} // namespace vistle
