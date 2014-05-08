/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <boost/foreach.hpp>
#include <boost/asio.hpp>

#include <mpi.h>

#include <sys/types.h>

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <core/message.h>
#include <core/tcpmessage.h>
#include <core/object.h>
#include <core/parameter.h>
#include <util/findself.h>

#include "communicator.h"
#include "modulemanager.h"

#define CERR \
   std::cerr << "comm [" << m_rank << "/" << m_size << "] "

using namespace boost::interprocess;
namespace asio = boost::asio;

namespace vistle {

enum MpiTags {
   TagModulue,
   TagToRank0,
   TagToAny,
};

Communicator *Communicator::s_singleton = NULL;

Communicator::Communicator(int argc, char *argv[], int r, const std::vector<std::string> &hosts)
: m_moduleManager(new ModuleManager(argc, argv, r, hosts))
, m_rank(r)
, m_size(hosts.size())
, m_quitFlag(false)
, m_recvSize(0)
, m_traceMessages(0)
, m_hubSocket(m_ioService)
{
   assert(s_singleton == NULL);
   s_singleton = this;

   CERR << "started" << std::endl;

   message::DefaultSender::init(0, m_rank);

   m_recvBufToAny.resize(message::Message::MESSAGE_SIZE);

   // post requests for length of next MPI message
   MPI_Irecv(&m_recvSize, 1, MPI_INT, MPI_ANY_SOURCE, TagToAny, MPI_COMM_WORLD, &m_reqAny);

   if (m_rank == 0) {
      m_recvBufTo0.resize(message::Message::MESSAGE_SIZE);
      MPI_Irecv(m_recvBufTo0.data(), m_recvBufTo0.size(), MPI_BYTE, MPI_ANY_SOURCE, TagToRank0, MPI_COMM_WORLD, &m_reqToRank0);
   }
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

bool Communicator::connectHub(const std::string &host, unsigned short port) {

   int ret = 1;
   if (getRank() == 0) {

      CERR << "connecting to hub on " << host << ":" << port << "..." << std::endl;
      std::stringstream portstr;
      portstr << port;

      asio::ip::tcp::resolver resolver(m_ioService);
      asio::ip::tcp::resolver::query query(host, portstr.str());
      asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
      boost::system::error_code ec;
      asio::connect(m_hubSocket, endpoint_iterator, ec);
      if (ec) {
         CERR << " could not establish connection to " << host << ":" << port << std::endl;
         ret = 0;
      }
   }

   MPI_Bcast(&ret, 1, MPI_INT, 0, MPI_COMM_WORLD);

   return ret;
}

bool Communicator::sendHub(const message::Message &message) {

   if (getRank() == 0)
      return message::send(m_hubSocket, message);
   else
      return true;
}


bool Communicator::scanModules(const std::string &dir) {

    return m_moduleManager->scanModules(dir);
}

void Communicator::setQuitFlag() {

   m_quitFlag = true;
}

bool Communicator::dispatch() {

   bool done = false;

   bool received = false;
   do {
      received = false;

      // check for new UIs and other network clients
      if (m_rank == 0) {

         if (!done)
            done = m_quitFlag;

         if (done) {
            sendHub(message::Quit());
         }
      }

      // handle or broadcast messages received from slaves (m_rank > 0)
      if (m_rank == 0) {
         int flag;
         MPI_Status status;
         MPI_Test(&m_reqToRank0, &flag, &status);
         if (flag && status.MPI_TAG == TagToRank0) {

            assert(m_rank == 0);
            received = true;
            message::Message *message = (message::Message *) m_recvBufTo0.data();
            if (message->broadcast()) {
               if (!broadcastAndHandleMessage(*message))
                  done = true;
            }  else {
               if (!handleMessage(*message))
                  done = true;
            }
            MPI_Irecv(m_recvBufTo0.data(), m_recvBufTo0.size(), MPI_BYTE, MPI_ANY_SOURCE, TagToRank0, MPI_COMM_WORLD, &m_reqToRank0);
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

         received = true;
         MPI_Bcast(m_recvBufToAny.data(), m_recvSize, MPI_BYTE,
                   status.MPI_SOURCE, MPI_COMM_WORLD);

         message::Message *message = (message::Message *) m_recvBufToAny.data();
#if 0
         printf("[%02d] message from [%02d] message type %d m_size %d\n",
                m_rank, status.MPI_SOURCE, message->getType(), mpiMessageSize);
#endif
         if (!handleMessage(*message))
            done = true;

         MPI_Irecv(&m_recvSize, 1, MPI_INT, MPI_ANY_SOURCE, TagToAny, MPI_COMM_WORLD, &m_reqAny);
      }

      if (m_rank == 0) {
         m_ioService.poll();
         bool received = false;
         std::vector<char> buf(message::Message::MESSAGE_SIZE);
         message::Message *message = (message::Message *)buf.data();
         if (!message::recv(m_hubSocket, *message, received)) {
            done = true;
         } else if (received) {
            if(!broadcastAndHandleMessage(*message))
               done = true;
         }
      }

      // test for messages from modules
      if (done) {
         if (m_moduleManager) {
            m_moduleManager->quit();
            m_quitFlag = false;
            done = false;
         }
      }
      if (m_moduleManager->quitOk()) {
         done = true;
      }
      if (!done && m_moduleManager) {
         done = !m_moduleManager->dispatch(received);
      }
      if (done) {
         delete m_moduleManager;
         m_moduleManager = nullptr;
      }

   } while (!done && received);

   return !done;
}

bool Communicator::sendMessage(const int moduleId, const message::Message &message) const {

   return moduleManager().sendMessage(moduleId, message);
}

bool Communicator::forwardToMaster(const message::Message &message) {

   assert(m_rank != 0);
   if (m_rank != 0) {

      MPI_Send(const_cast<message::Message *>(&message), message.m_size, MPI_BYTE, 0, TagToRank0, MPI_COMM_WORLD);
   }

   return true;
}

bool Communicator::broadcastAndHandleMessage(const message::Message &message) {

   if (m_rank == 0) {
      std::vector<MPI_Request> s(m_size);
      for (int index = 0; index < m_size; ++index) {
         if (index != m_rank)
            MPI_Isend(const_cast<unsigned int *>(&message.m_size), 1, MPI_UNSIGNED, index, TagToAny,
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
      MPI_Send(const_cast<message::Message *>(&message), message.m_size, MPI_BYTE, 0, TagToRank0, MPI_COMM_WORLD);

      // message will be handled when received again from m_rank 0
      return true;
   }
}


bool Communicator::handleMessage(const message::Message &message) {

   using namespace vistle::message;

   if (m_traceMessages == -1 || message.type() == m_traceMessages) {
      CERR << "Message: " << message << std::endl;
   }

   bool result = true;
   switch (message.type()) {
      case Message::IDENTIFY: {

         const Identify &id = static_cast<const message::Identify &>(message);
         sendHub(Identify(Identify::MANAGER));
         auto avail = moduleManager().availableModules();
         for(const auto &mod: avail) {
            sendHub(message::ModuleAvailable(mod.name, mod.path));
         }
         break;
      }

      case message::Message::PING: {

         const message::Ping &ping = static_cast<const message::Ping &>(message);
         sendHub(ping);
         result = m_moduleManager->handle(ping);
         break;
      }

      case message::Message::PONG: {

         const message::Pong &pong = static_cast<const message::Pong &>(message);
         sendHub(pong);
         result = m_moduleManager->handle(pong);
         break;
      }

      case message::Message::TRACE: {
         const Trace &trace = static_cast<const Trace &>(message);
         sendHub(trace);
         if (trace.module() <= 0) {
            if (trace.on())
               m_traceMessages = trace.messageType();
            else
               m_traceMessages = 0;
            result = true;
         }

         if (trace.module() > 0 || trace.module() == -1) {
            result = m_moduleManager->handle(trace);
         }
         break;
      }

      case message::Message::QUIT: {

         const message::Quit &quit = static_cast<const message::Quit &>(message);
         sendHub(quit);
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
         sendHub(started);
         result = m_moduleManager->handle(started);
         break;
      }

      case message::Message::KILL: {

         const message::Kill &kill = static_cast<const message::Kill &>(message);
         sendHub(kill);
         result = m_moduleManager->handle(kill);
         break;
      }

      case message::Message::CONNECT: {

         const message::Connect &connect = static_cast<const message::Connect &>(message);
         result = m_moduleManager->handle(connect);
         break;
      }

      case message::Message::DISCONNECT: {

         const message::Disconnect &disc = static_cast<const message::Disconnect &>(message);
         result = m_moduleManager->handle(disc);
         break;
      }

      case message::Message::MODULEEXIT: {

         const message::ModuleExit &moduleExit = static_cast<const message::ModuleExit &>(message);
         result = m_moduleManager->handle(moduleExit);
         sendHub(moduleExit);
         break;
      }

      case message::Message::COMPUTE: {

         const message::Compute &comp = static_cast<const message::Compute &>(message);
         result = m_moduleManager->handle(comp);
         break;
      }

      case message::Message::REDUCE: {
         const message::Reduce &red = static_cast<const message::Reduce &>(message);
         result = m_moduleManager->handle(red);
         break;
      }

      case message::Message::EXECUTIONPROGRESS: {

         const message::ExecutionProgress &prog = static_cast<const message::ExecutionProgress &>(message);
         result = m_moduleManager->handle(prog);
         break;
      }

      case message::Message::BUSY: {

         const message::Busy &busy = static_cast<const message::Busy &>(message);
         sendHub(busy);
         result = m_moduleManager->handle(busy);
         break;
      }

      case message::Message::IDLE: {

         const message::Idle &idle = static_cast<const message::Idle &>(message);
         sendHub(idle);
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
         sendHub(m);
         result = m_moduleManager->handle(m);
         break;
      }

      case message::Message::SETPARAMETERCHOICES: {

         const message::SetParameterChoices &m = static_cast<const message::SetParameterChoices &>(message);
         sendHub(m);
         result = m_moduleManager->handle(m);
         break;
      }

      case message::Message::ADDPARAMETER: {
         
         const message::AddParameter &m = static_cast<const message::AddParameter &>(message);
         sendHub(m);
         result = m_moduleManager->handle(m);
         break;
      }

      case message::Message::CREATEPORT: {

         const message::CreatePort &m = static_cast<const message::CreatePort &>(message);
         sendHub(m);
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
         result = m_moduleManager->handle(m);
         break;
      }

      case message::Message::RESETMODULEIDS: {
         const message::ResetModuleIds &m = static_cast<const message::ResetModuleIds &>(message);
         result = m_moduleManager->handle(m);
         break;
      }

      case message::Message::SENDTEXT: {
         const message::SendText &m = static_cast<const message::SendText &>(message);
         if (m_rank == 0) {
            sendHub(m);
         } else {
            result = forwardToMaster(m);
         }
         //result = m_moduleManager->handle(m);
         break;
      }

      case Message::OBJECTRECEIVEPOLICY: {
         const ObjectReceivePolicy &m = static_cast<const ObjectReceivePolicy &>(message);
         result = m_moduleManager->handle(m);
         break;
      }

      case Message::SCHEDULINGPOLICY: {
         const SchedulingPolicy &m = static_cast<const SchedulingPolicy &>(message);
         result = m_moduleManager->handle(m);
         break;
      }

      case Message::REDUCEPOLICY: {
         const ReducePolicy &m = static_cast<const ReducePolicy &>(message);
         result = m_moduleManager->handle(m);
         break;
      }

      case Message::MODULEAVAILABLE: {
         const ModuleAvailable &m = static_cast<const ModuleAvailable &>(message);
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

Communicator::~Communicator() {

   delete m_moduleManager;
   m_moduleManager = NULL;

   if (m_size > 1) {
      int dummy = 0;
      MPI_Request s;
      MPI_Isend(&dummy, 1, MPI_INT, (m_rank + 1) % m_size, TagToAny, MPI_COMM_WORLD, &s);
      if (m_rank == 1) {
         MPI_Request s2;
         MPI_Isend(&dummy, 1, MPI_BYTE, 0, TagToRank0, MPI_COMM_WORLD, &s2);
         //MPI_Wait(&s2, MPI_STATUS_IGNORE);
      }
      //MPI_Wait(&s, MPI_STATUS_IGNORE);
      //MPI_Wait(&m_reqAny, MPI_STATUS_IGNORE);
   }
   MPI_Barrier(MPI_COMM_WORLD);
}

ModuleManager &Communicator::moduleManager() const {

   return *m_moduleManager;
}

} // namespace vistle
