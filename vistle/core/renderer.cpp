#include "message.h"
#include "messagequeue.h"
#include "object.h"

#include "renderer.h"

#include <util/vecstreambuf.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace ba = boost::archive;

namespace vistle {

Renderer::Renderer(const std::string & name, const std::string &shmname,
                   const int rank, const int size, const int moduleID)
   : Module(name, shmname, rank, size, moduleID) {

   createInputPort("data_in", "input data");

   std::cerr << "Renderer starting: rank=" << rank << std::endl;
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

   int sync = 0, allsync = 0;

   if (msg) {
      vistle::message::Message *message = (vistle::message::Message *) msgRecvBuf;

      switch (message->type()) {
         case vistle::message::Message::OBJECTRECEIVED:
         case vistle::message::Message::QUIT:
            sync = 1;
            break;
         default:
            break;
      }
   }

   MPI_Allreduce(&sync, &allsync, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

   do {
      if (msg) {
         vistle::message::Message *message = (vistle::message::Message *) msgRecvBuf;

         switch (message->type()) {
            case vistle::message::Message::OBJECTRECEIVED: {
               const message::ObjectReceived *recv = static_cast<const message::ObjectReceived *>(message);
               if (size() > 1) {
                  if (recv->rank() == rank()) {
                     Object::const_ptr obj = Shm::the().getObjectFromName(recv->objectName());
                     vecstreambuf<char> memstr;
                     ba::binary_oarchive memar(memstr);
                     obj->save(memar);
                     const std::vector<char> &mem = memstr.get_vector();
                     uint64_t len = mem.size();
                     std::cerr << "Rank " << rank() << ": Broadcasting " << len << " bytes, type=" << obj->getType() << " (" << obj->getName() << ")" << std::endl;
                     const char *data = mem.data();
                     MPI_Bcast(&len, 1, MPI_UINT64_T, rank(), MPI_COMM_WORLD);
                     MPI_Bcast(const_cast<char *>(data), len, MPI_BYTE, rank(), MPI_COMM_WORLD);
                  } else {
                     uint64_t len = 0;
                     std::cerr << "Rank " << rank() << ": Waiting to receive" << std::endl;
                     MPI_Bcast(&len, 1, MPI_UINT64_T, recv->rank(), MPI_COMM_WORLD);
                     std::cerr << "Rank " << rank() << ": Waiting to receive " << len << " bytes" << std::endl;
                     std::vector<char> mem(len);
                     char *data = mem.data();
                     MPI_Bcast(data, mem.size(), MPI_BYTE, recv->rank(), MPI_COMM_WORLD);
                     std::cerr << "Rank " << rank() << ": Received " << len << " bytes for " << recv->objectName() << std::endl;
                     vecstreambuf<char> membuf(mem);
                     ba::binary_iarchive memar(membuf);
                     Object::ptr obj = Object::load(memar);
                     if (obj) {
                        std::cerr << "Rank " << rank() << ": Restored " << recv->objectName() << " as " << obj->getName() << ", type: " << obj->getType() << std::endl;
                        assert(obj->check());
                        addInputObject(recv->getPortName(), obj);
                     }
                  }
               }
               break;
            }
            default:
               again = handleMessage(message);
               break;
         }

         switch (message->type()) {
            case vistle::message::Message::OBJECTRECEIVED:
            case vistle::message::Message::QUIT:
               sync = 1;
               break;
            default:
               break;
         }

      }

      if (allsync && !sync) {
         receiveMessageQueue->getMessageQueue().receive(
               (void *) msgRecvBuf,
               message::Message::MESSAGE_SIZE,
               msgSize, priority);
         msg = true;
      }


   } while(allsync && !sync);

   if (!again) {
      vistle::message::ModuleExit m;
      sendMessageQueue->getMessageQueue().send(&m, sizeof(m), 0);
   }

   if (again)
      render();

   return again;
}

} // namespace vistle
