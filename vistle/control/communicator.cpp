/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <boost/foreach.hpp>

#include <mpi.h>

#include <sys/types.h>

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <core/message.h>
#include <core/messagequeue.h>
#include <core/object.h>
#include <core/parameter.h>
#include <util/findself.h>

#include "clientmanager.h"
#include "uimanager.h"
#include "communicator.h"
#include "modulemanager.h"

#define CERR \
   std::cerr << "comm [" << m_rank << "/" << m_size << "] "

//#define DEBUG

using namespace boost::interprocess;

namespace vistle {

enum MpiTags {
   TagModulue,
   TagToRank0,
   TagToAny,
};

Communicator *Communicator::s_singleton = NULL;

Communicator::Communicator(int argc, char *argv[], int r, const std::vector<std::string> &hosts)
: m_clientManager(NULL)
, m_uiManager(NULL)
, m_moduleManager(new ModuleManager(argc, argv, r, hosts))
, m_rank(r)
, m_size(hosts.size())
, m_quitFlag(false)
, mpiReceiveBuffer(NULL)
, mpiMessageSize(0)
, m_commandQueue(message::MessageQueue::create(message::MessageQueue::createName("command_queue", 0, r)))
{
   assert(s_singleton == NULL);
   s_singleton = this;

   message::DefaultSender::init(0, m_rank);

   mpiReceiveBuffer = new char[message::Message::MESSAGE_SIZE];

   // post requests for length of next MPI message
   MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, TagToAny, MPI_COMM_WORLD, &m_reqAny);
   if (m_rank == 0)
      MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, TagToRank0, MPI_COMM_WORLD, &m_reqToRank0);
}

Communicator &Communicator::the() {

   assert(s_singleton && "make sure to use the vistle Python module only from within vistle");
   if (!s_singleton)
      exit(1);
   return *s_singleton;
}

int Communicator::getRank() const {

   return m_rank;
}

int Communicator::getSize() const {

   return m_size;
}

void Communicator::setInput(const std::string &input) {

   m_initialInput = input;
}

void Communicator::setFile(const std::string &filename) {

   m_initialFile = filename;
}

void Communicator::setQuitFlag() {

   m_quitFlag = true;
}

bool Communicator::dispatch() {

   bool done = false;

   if (m_rank == 0) {

      if (!m_clientManager) {
         bool file = !m_initialFile.empty();
         m_clientManager = new ClientManager(file ? m_initialFile : m_initialInput,
               file ? ClientManager::File : ClientManager::String);
      }

      done = !m_clientManager->check();

      if (!done)
         done = m_quitFlag;

      if (!m_uiManager) {
         m_uiManager = new UiManager(m_commandQueue);
      }

      if (!done) 
         done = !m_uiManager->check();
   }

   // broadcast messages received from slaves (m_rank > 0)
   if (m_rank == 0) {
      int flag;
      MPI_Status status;
      MPI_Test(&m_reqToRank0, &flag, &status);
      if (flag && status.MPI_TAG == TagToRank0) {

         assert(m_rank == 0);
         MPI_Request r;
         MPI_Irecv(mpiReceiveBuffer, mpiMessageSize, MPI_BYTE, status.MPI_SOURCE, TagToRank0,
               MPI_COMM_WORLD, &r);
         MPI_Wait(&r, MPI_STATUS_IGNORE);
         message::Message *message = (message::Message *) mpiReceiveBuffer;
         if (!broadcastAndHandleMessage(*message))
            done = true;
         MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, TagToRank0, MPI_COMM_WORLD, &m_reqToRank0);
      }
   }

   // test for message m_size from another MPI node
   //    - receive actual message from broadcast (on any m_rank)
   //    - receive actual message from slave m_rank (on m_rank 0) for broadcasting
   //    - handle message
   //    - post another MPI receive for m_size of next message
   int flag;
   MPI_Status status;
   MPI_Test(&m_reqAny, &flag, &status);
   if (flag && status.MPI_TAG == TagToAny) {

      MPI_Bcast(mpiReceiveBuffer, mpiMessageSize, MPI_BYTE,
            status.MPI_SOURCE, MPI_COMM_WORLD);

      message::Message *message = (message::Message *) mpiReceiveBuffer;
#if 0
      printf("[%02d] message from [%02d] message type %d m_size %d\n",
            m_rank, status.MPI_SOURCE, message->getType(), mpiMessageSize);
#endif
      if (!handleMessage(*message))
         done = true;

      MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, TagToAny, MPI_COMM_WORLD, &m_reqAny);
   }

   // test for messages from modules
   bool received = false;
   do {
      received = false;
      // handle messages from Python front-end and UIs
      if (!tryReceiveAndHandleMessage(m_commandQueue->getMessageQueue(), received, true))
         done = true;

      if (!done)
         done = !m_moduleManager->dispatch(received);

   } while (received);

   return !done;
}

bool Communicator::tryReceiveAndHandleMessage(boost::interprocess::message_queue &mq, bool &received, bool broadcast) {

   bool done = false;
   try {
      size_t msgSize;
      unsigned int priority;
      char msgRecvBuf[vistle::message::Message::MESSAGE_SIZE];

      received = mq.try_receive(
            (void *) msgRecvBuf,
            vistle::message::Message::MESSAGE_SIZE,
            msgSize, priority);

      if (received) {
         message::Message *msg = reinterpret_cast<message::Message *>(msgRecvBuf);
         if (broadcast) {
            if (!broadcastAndHandleMessage(*msg))
               done = true;
         } else {
            if (!handleMessage(*msg))
               done = true;
         }
         return !done;
      }
   } catch (interprocess_exception &ex) {
      CERR << "receive mq " << ex.what() << std::endl;
      exit(-1);
   }

   return !done;
}

bool Communicator::sendMessage(const int moduleId, const message::Message &message) const {

   return moduleManager().sendMessage(moduleId, message);
}

bool Communicator::broadcastAndHandleMessage(const message::Message &message) {

   if (m_rank == 0) {
      std::vector<MPI_Request> s(m_size);
      for (int index = 0; index < m_size; ++index) {
         if (index != m_rank)
            MPI_Isend(const_cast<unsigned int *>(&message.m_size), 1, MPI_INT, index, TagToAny,
                  MPI_COMM_WORLD, &s[index]);
      }

      MPI_Bcast(const_cast<message::Message *>(&message), message.m_size, MPI_BYTE, m_rank, MPI_COMM_WORLD);

      const bool result = handleMessage(message);

      // wait for completion
      for (int index=0; index<m_size; ++index) {

         if (index == m_rank)
            continue;

         MPI_Wait(&s[index], MPI_STATUS_IGNORE);
      }

      return result;
   } else {
      MPI_Request s1, s2;
      MPI_Isend(const_cast<unsigned int *>(&message.m_size), 1, MPI_INT, 0, TagToRank0, MPI_COMM_WORLD, &s1);
      MPI_Isend(const_cast<message::Message *>(&message), message.m_size, MPI_BYTE, 0, TagToRank0, MPI_COMM_WORLD, &s2);
      MPI_Wait(&s1, MPI_STATUS_IGNORE);
      MPI_Wait(&s2, MPI_STATUS_IGNORE);

      // message will be handled when received again from m_rank 0
      return true;
   }
}


bool Communicator::handleMessage(const message::Message &message) {

#ifdef DEBUG
   CERR << "Message: "
      << "type=" << message.type() << ", "
      << "size=" << message.size() << ", "
      << "from=" << message.senderId() << ", "
      << "m_rank=" << message.rank() << std::endl;
#endif

   bool result = true;
   switch (message.type()) {

      case message::Message::PING: {

         const message::Ping &ping = static_cast<const message::Ping &>(message);
         sendUi(ping);
         result = m_moduleManager->handle(ping);
         break;
      }

      case message::Message::PONG: {

         const message::Pong &pong = static_cast<const message::Pong &>(message);
         sendUi(pong);
         result = m_moduleManager->handle(pong);
         break;
      }

      case message::Message::QUIT: {

         const message::Quit &quit = static_cast<const message::Quit &>(message);
         sendUi(quit);
         result = false;
         break;
      }

      case message::Message::SPAWN: {

         const message::Spawn &spawn = static_cast<const message::Spawn &>(message);
         result = m_moduleManager->handle(spawn);
         break;
      }

      case message::Message::STARTED: {

         const message::Started &started = static_cast<const message::Started &>(message);
         sendUi(started);
         result = m_moduleManager->handle(started);
         break;
      }

      case message::Message::KILL: {

         const message::Kill &kill = static_cast<const message::Kill &>(message);
         sendUi(kill);
         result = m_moduleManager->handle(kill);
         break;
      }

      case message::Message::CONNECT: {

         const message::Connect &connect = static_cast<const message::Connect &>(message);
         sendUi(connect);
         result = m_moduleManager->handle(connect);
         break;
      }

      case message::Message::DISCONNECT: {

         const message::Disconnect &disc = static_cast<const message::Disconnect &>(message);
         sendUi(disc);
         result = m_moduleManager->handle(disc);
         break;
      }

      case message::Message::NEWOBJECT: {

         /*
         const message::NewObject *newObject =
            static_cast<const message::NewObject *>(message);
         vistle::Object *object = (vistle::Object *)
            vistle::Shm::the().shm().get_address_from_handle(newObject.getHandle());

         CERR << "NewObject ["
                   << newObject.getHandle() << "] type ["
                   << object.getType() << "] from module ["
                   << newObject.senderId() << "]" << std::endl;
         */
         break;
      }

      case message::Message::MODULEEXIT: {

         const message::ModuleExit &moduleExit = static_cast<const message::ModuleExit &>(message);
         sendUi(moduleExit);
         result = m_moduleManager->handle(moduleExit);
         break;
      }

      case message::Message::COMPUTE: {

         const message::Compute &comp = static_cast<const message::Compute &>(message);
         result = m_moduleManager->handle(comp);
         break;
      }

      case message::Message::BUSY: {

         const message::Busy &busy = static_cast<const message::Busy &>(message);
         sendUi(busy);
         result = m_moduleManager->handle(busy);
         break;
      }

      case message::Message::IDLE: {

         const message::Idle &idle = static_cast<const message::Idle &>(message);
         sendUi(idle);
         result = m_moduleManager->handle(idle);
         break;
      }

      case message::Message::ADDOBJECT: {

         const message::AddObject &m = static_cast<const message::AddObject &>(message);
         result = m_moduleManager->handle(m);
         break;
      }

      case message::Message::OBJECTRECEIVED: {
         const message::ObjectReceived &m = static_cast<const message::ObjectReceived &>(message);
         result = m_moduleManager->handle(m);
         break;
      }

      case message::Message::SETPARAMETER: {

         const message::SetParameter &m = static_cast<const message::SetParameter &>(message);
         sendUi(m);
         result = m_moduleManager->handle(m);
         break;
      }

      case message::Message::ADDPARAMETER: {
         
         const message::AddParameter &m = static_cast<const message::AddParameter &>(message);
         sendUi(m);
         result = m_moduleManager->handle(m);
         break;
      }

      case message::Message::CREATEPORT: {

         const message::CreatePort &m = static_cast<const message::CreatePort &>(message);
         sendUi(m);
         result = m_moduleManager->handle(m);
         break;
      }

      case message::Message::BARRIER: {

         const message::Barrier &m = static_cast<const message::Barrier &>(message);
         result = m_moduleManager->handle(m);
         break;
      }

      case message::Message::BARRIERREACHED: {

         const message::BarrierReached &m = static_cast<const message::BarrierReached &>(message);
         sendUi(m);
         result = m_moduleManager->handle(m);
         break;
      }

      default:

         CERR << "unhandled message from (id "
            << message.senderId() << " m_rank " << message.rank() << ") "
            << "type " << message.type()
            << std::endl;

         break;

   }

   return result;
}

void Communicator::sendUi(const message::Message &msg) const {

   if (m_uiManager)
      m_uiManager->sendMessage(msg);
}

Communicator::~Communicator() {

   delete m_uiManager;
   m_uiManager = NULL;
   delete m_moduleManager;
   m_moduleManager = NULL;
   delete m_clientManager;
   m_clientManager = NULL;

   if (m_size > 1) {
      int dummy = 0;
      MPI_Request s;
      MPI_Isend(&dummy, 1, MPI_INT, (m_rank + 1) % m_size, TagToAny, MPI_COMM_WORLD, &s);
      if (m_rank == 1) {
         MPI_Request s2;
         MPI_Isend(&dummy, 1, MPI_INT, 0, TagToRank0, MPI_COMM_WORLD, &s2);
         //MPI_Wait(&s2, MPI_STATUS_IGNORE);
      }
      //MPI_Wait(&s, MPI_STATUS_IGNORE);
      //MPI_Wait(&m_reqAny, MPI_STATUS_IGNORE);
   }
   MPI_Barrier(MPI_COMM_WORLD);

   delete[] mpiReceiveBuffer;
}

ModuleManager &Communicator::moduleManager() const {

   return *m_moduleManager;
}

} // namespace vistle
