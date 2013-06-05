#include "message.h"
#include "messagequeue.h"

#include "renderer.h"

#include <streambuf>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <core/object.h>

namespace ba = boost::archive;

namespace vistle {

class vecstreambuf: public std::streambuf {
 public:
   vecstreambuf(std::vector<char> &vec) 
   : m_vector(vec) {
      setg(vec.data(), vec.data(), vec.data()+vec.size());
   }
   vecstreambuf() {
   }
   int_type overflow(int_type ch) {
      if (ch != EOF) {
         m_vector.push_back(ch);
         return 0;
      } else {
         return EOF;
      }
   }
   std::streamsize xsputn (const char *s, std::streamsize num) {
      size_t oldsize = m_vector.size();
      m_vector.resize(oldsize+num);
      memcpy(m_vector.data()+oldsize, s, num);
      return num;
    }
   const std::vector<char> &get_vector() const {
      return m_vector;
   }
 private:
   std::vector<char> m_vector;

};

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
               std::cerr << "*****" << std::endl;
               std::cerr << "***** ObjectReceived: rank " << rank() << " from " << recv->rank() << std::endl;
#if 1
               if (recv->rank() == rank()) {
                  Object::const_ptr obj = Shm::the().getObjectFromName(recv->objectName());
                  vecstreambuf memstr;
                  ba::binary_oarchive memar(memstr);
                  obj->save(memar);
                  const std::vector<char> &mem = memstr.get_vector();
                  uint64_t len = mem.size();
                  std::cerr << "Rank " << rank() << ": Broadcasting " << len << " bytes, type=" << obj->getType() << " (" << obj->getName() << ")" << std::endl;
                  const char *data = mem.data();
                  std::vector<MPI_Request> request(size());
                  for (int r=0; r<size(); ++r) {
                     if (r == rank())
                        continue;
                     MPI_Isend(&len, 1, MPI_UINT64_T, r, 3453, MPI_COMM_WORLD, &request[r]);
                  }
                  //MPI_Bcast(&len, 1, MPI_UINT64_T, rank(), MPI_COMM_WORLD);
                  MPI_Bcast(const_cast<char *>(data), len, MPI_BYTE, rank(), MPI_COMM_WORLD);
                  for (int r=0; r<size(); ++r) {
                     if (r == rank())
                        continue;
                     MPI_Wait(&request[r], MPI_STATUS_IGNORE);
                  }
               } else {
                  uint64_t len = 0;
                  std::cerr << "Rank " << rank() << ": Waiting to receive" << std::endl;
                  //MPI_Bcast(&len, 1, MPI_UINT64_T, recv->rank(), MPI_COMM_WORLD);
                  MPI_Request r;
                  MPI_Irecv(&len, 1, MPI_UINT64_T, recv->rank(), 3453, MPI_COMM_WORLD, &r);
                  MPI_Wait(&r, MPI_STATUS_IGNORE);
                  std::cerr << "Rank " << rank() << ": Waiting to receive " << len << " bytes" << std::endl;
                  std::vector<char> mem(len);
                  char *data = mem.data();
                  MPI_Bcast(data, mem.size(), MPI_BYTE, recv->rank(), MPI_COMM_WORLD);
                  std::cerr << "Rank " << rank() << ": Received " << len << " bytes for " << recv->objectName() << std::endl;
                  vecstreambuf membuf(mem);
                  ba::binary_iarchive memar(membuf);
                  Object::ptr obj = Object::load(memar);
                  if (obj) {
                     std::cerr << "Rank " << rank() << ": Restored " << recv->objectName() << " as " << obj->getName() << ", type: " << obj->getType() << std::endl;
                     assert(obj->check());
                     addInputObject(recv->getPortName(), obj);
                  }
               }
#endif
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
