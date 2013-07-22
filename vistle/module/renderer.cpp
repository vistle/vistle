#include <core/message.h>
#include <core/messagequeue.h>
#include <core/object.h>
#include <core/placeholder.h>

#include "renderer.h"

#include <util/vecstreambuf.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace ba = boost::archive;

namespace vistle {

const int MaxObjectsPerFrame = 50;

Renderer::Renderer(const std::string & name, const std::string &shmname,
                   const int rank, const int size, const int moduleID)
   : Module(name, shmname, rank, size, moduleID) {

   createInputPort("data_in", "input data");

   m_masterOnly = addIntParameter("master_only", "render only on master (rank 0)", 0);

   //std::cerr << "Renderer starting: rank=" << rank << std::endl;
}

Renderer::~Renderer() {

}

bool Renderer::dispatch() {

   size_t msgSize;
   unsigned int priority;
   char msgRecvBuf[message::Message::MESSAGE_SIZE];

   bool quit = false;
   bool checkAgain = false;
   int numSync = 0;
   do {
      bool haveMessage = receiveMessageQueue->getMessageQueue().try_receive(
                                               (void *) msgRecvBuf,
                                               message::Message::MESSAGE_SIZE,
                                               msgSize, priority);

      int sync = 0, allsync = 0;

      if (haveMessage) {
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
         if (haveMessage) {
            vistle::message::Message *message = (vistle::message::Message *) msgRecvBuf;

            switch (message->type()) {
               case vistle::message::Message::ADDOBJECT: {
                  if (size() == 1) {
                     const message::AddObject *add = static_cast<const message::AddObject *>(message);
                     addInputObject(add->getPortName(), add->takeObject());
                  }
                  break;
               }
               case vistle::message::Message::OBJECTRECEIVED: {
                  if (size() > 1) {
                     const message::ObjectReceived *recv = static_cast<const message::ObjectReceived *>(message);
                     PlaceHolder::ptr ph(new PlaceHolder(recv->objectName(), recv->meta(), recv->objectType()));
                     const bool bcast = !m_masterOnly->getValue();
                     const bool localAdd = !m_masterOnly->getValue() || rank()==0 || recv->objectType() == Object::GEOMETRY;
                     if (recv->rank() == rank()) {
                        Object::const_ptr obj = Shm::the().getObjectFromName(recv->objectName());
                        if (obj) {
                           vecstreambuf<char> memstr;
                           ba::binary_oarchive memar(memstr);
                           obj->save(memar);
                           const std::vector<char> &mem = memstr.get_vector();
                           uint64_t len = mem.size();
                           //std::cerr << "Rank " << rank() << ": Broadcasting " << len << " bytes, type=" << obj->getType() << " (" << obj->getName() << ")" << std::endl;
                           const char *data = mem.data();
                           if (bcast) {
                              MPI_Bcast(&len, 1, MPI_UINT64_T, rank(), MPI_COMM_WORLD);
                              MPI_Bcast(const_cast<char *>(data), len, MPI_BYTE, rank(), MPI_COMM_WORLD);
                           } else if (rank() != 0) {
                              MPI_Request r1, r2;
                              MPI_Isend(&len, 1, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD, &r1);
                              MPI_Isend(const_cast<char *>(data), len, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &r2);
                              MPI_Wait(&r1, MPI_STATUS_IGNORE);
                              MPI_Wait(&r2, MPI_STATUS_IGNORE);
                           }
                        } else {
                           uint64_t len = 0;
                           if (bcast) {
                              MPI_Bcast(&len, 1, MPI_UINT64_T, rank(), MPI_COMM_WORLD);
                           } else if (rank() != 0) {
                              MPI_Request r;
                              MPI_Isend(&len, 1, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD, &r);
                              MPI_Wait(&r, MPI_STATUS_IGNORE);
                           }
                           std::cerr << "Rank " << rank() << ": OBJECT NOT FOUND: " << recv->objectName() << std::endl;
                        }
                        assert(obj->check());
                        if (localAdd) {
                           addInputObject(recv->getPortName(), obj);
                        }
                        obj->unref(); // normally done in AddObject::takeObject();
                     } else {
                        uint64_t len = 0;
                        //std::cerr << "Rank " << rank() << ": Waiting to receive" << std::endl;
                        if (bcast) {
                           MPI_Bcast(&len, 1, MPI_UINT64_T, recv->rank(), MPI_COMM_WORLD);
                        } else if (rank() == 0) {
                           MPI_Request r;
                           MPI_Irecv(&len, 1, MPI_UINT64_T, recv->rank(), 0, MPI_COMM_WORLD, &r);
                           MPI_Wait(&r, MPI_STATUS_IGNORE);
                        }
                        if (len > 0) {
                           //std::cerr << "Rank " << rank() << ": Waiting to receive " << len << " bytes" << std::endl;
                           std::vector<char> mem(len);
                           char *data = mem.data();
                           if (bcast) {
                              MPI_Bcast(data, mem.size(), MPI_BYTE, recv->rank(), MPI_COMM_WORLD);
                           } else if (rank() == 0) {
                              MPI_Request r;
                              MPI_Irecv(data, mem.size(), MPI_BYTE, recv->rank(), 0, MPI_COMM_WORLD, &r);
                              MPI_Wait(&r, MPI_STATUS_IGNORE);
                           }
                           //std::cerr << "Rank " << rank() << ": Received " << len << " bytes for " << recv->objectName() << std::endl;
                           vecstreambuf<char> membuf(mem);
                           ba::binary_iarchive memar(membuf);
                           Object::ptr obj = Object::load(memar);
                           if (obj) {
                              //std::cerr << "Rank " << rank() << ": Restored " << recv->objectName() << " as " << obj->getName() << ", type: " << obj->getType() << std::endl;
                              assert(obj->check());
                              if (localAdd) {
                                 addInputObject(recv->getPortName(), obj);
                              }
                           }
                        }
                     }
                     if (!localAdd)
                        addInputObject(recv->getPortName(), ph);
                  }
                  break;
               }
               default:
                  quit = !handleMessage(message);
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
            haveMessage = true;
         }

      } while(allsync && !sync);

      int numMessages = receiveMessageQueue->getMessageQueue().get_num_msg();
      int maxNumMessages = 0;
      MPI_Allreduce(&numMessages, &maxNumMessages, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
      ++numSync;
      checkAgain = maxNumMessages>0 && numSync<MaxObjectsPerFrame;

   } while (checkAgain && !quit);

   if (quit) {
      vistle::message::ModuleExit m;
      sendMessageQueue->getMessageQueue().send(&m, sizeof(m), 0);
   } else {
      render();
   }

   return !quit;
}

} // namespace vistle
