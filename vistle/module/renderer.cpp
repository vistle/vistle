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

Renderer::Renderer(const std::string &description, const std::string &shmname,
                   const std::string &name, const int moduleID)
   : Module(description, shmname, name, moduleID) {

   createInputPort("data_in", "input data");

   m_renderMode = addIntParameter("render_mode", "Render on which nodes?", 1, Parameter::Choice);
   std::vector<std::string> choices;
   choices.push_back("All nodes");
   choices.push_back("Local only");
   choices.push_back("Master only");
   setParameterChoices(m_renderMode, choices);

   //std::cerr << "Renderer starting: rank=" << rank << std::endl;
}

Renderer::~Renderer() {

}

static bool needsSync(const message::Message &m) {

   switch (m.type()) {
      case vistle::message::Message::OBJECTRECEIVED:
      case vistle::message::Message::QUIT:
      case vistle::message::Message::KILL:
         return true;
      default:
         return false;
   }
}

bool Renderer::dispatch() {

   char msgRecvBuf[message::Message::MESSAGE_SIZE];
   vistle::message::Message *message = (vistle::message::Message *) msgRecvBuf;

   int quit = 0;
   bool checkAgain = false;
   int numSync = 0;
   do {
      bool haveMessage = receiveMessageQueue->tryReceive(*message);

      int sync = 0, allsync = 0;

      if (haveMessage) {
         if (needsSync(*message))
            sync = 1;
      }

      MPI_Allreduce(&sync, &allsync, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

      do {
         if (haveMessage) {

            switch (message->type()) {
               case vistle::message::Message::ADDOBJECT: {
                  if (size() == 1 || objectReceivePolicy()==message::ObjectReceivePolicy::Single) {
                     const message::AddObject *add = static_cast<const message::AddObject *>(message);
                     addInputObject(add->getPortName(), add->takeObject());
                  }
                  break;
               }
               case vistle::message::Message::OBJECTRECEIVED: {
                  vassert(objectReceivePolicy() != message::ObjectReceivePolicy::Single);
                  if (size() > 1) {
                     const message::ObjectReceived *recv = static_cast<const message::ObjectReceived *>(message);
                     PlaceHolder::ptr ph(new PlaceHolder(recv->objectName(), recv->meta(), recv->objectType()));
                     const bool send = m_renderMode->getValue() != 1;
                     const bool bcast = m_renderMode->getValue() == 0;
                     bool localAdd = ((send || bcast) && m_rank==0) || (m_renderMode->getValue()==1 && recv->rank() == rank());
                     if (recv->rank() == rank()) {
                        Object::const_ptr obj = Shm::the().getObjectFromName(recv->objectName());
                        if (send) {
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
                        }
                        vassert(obj->check());
                        if (localAdd) {
                           addInputObject(recv->getPortName(), obj);
                        }
                        obj->unref(); // normally done in AddObject::takeObject();
                     } else {
                        if (send) {
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
                                 vassert(obj->check());
                                 if (localAdd) {
                                    addInputObject(recv->getPortName(), obj);
                                 }
                              } else {
                                 localAdd = false;
                              }
                           } else {
                              localAdd = false;
                           }
                        } else {
                           localAdd = false;
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

            if (needsSync(*message))
               sync = 1;
         }

         if (allsync && !sync) {
            receiveMessageQueue->receive(*message);
            haveMessage = true;
         }

      } while(allsync && !sync);

      int numMessages = receiveMessageQueue->getNumMessages();
      int maxNumMessages = 0;
      MPI_Allreduce(&numMessages, &maxNumMessages, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
      ++numSync;
      checkAgain = maxNumMessages>0 && numSync<MaxObjectsPerFrame;

   } while (checkAgain && !quit);

   int doQuit = 0;
   MPI_Allreduce(&quit, &doQuit, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
   if (doQuit) {
      vistle::message::ModuleExit m;
      sendMessageQueue->send(m);
   } else {
      render();
   }

   return !quit;
}

} // namespace vistle
