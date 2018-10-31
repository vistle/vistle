/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <boost/asio.hpp>
#include <boost/mpi.hpp>

#include <mpi.h>

#include <sys/types.h>

#include <cstdlib>
#include <iostream>
#include <iomanip>

#include <core/message.h>
#include <core/tcpmessage.h>
#include <core/object.h>
#include <core/parameter.h>
#include <core/assert.h>
#include <util/sleep.h>
#include <util/tools.h>
#include <util/hostname.h>

#include "communicator.h"
#include "clustermanager.h"
#include "datamanager.h"

#if defined(MODULE_THREAD) && defined(MODULE_STATIC)
#include <module/moduleregistry.h>
#endif

#define CERR \
   std::cerr << "comm [" << m_rank << "/" << m_size << "] "

using namespace boost::interprocess;
namespace asio = boost::asio;
namespace mpi = boost::mpi;

namespace vistle {

using message::Id;

Communicator *Communicator::s_singleton = NULL;

Communicator::Communicator(int r, const std::vector<std::string> &hosts)
: m_comm(MPI_COMM_WORLD, mpi::comm_attach)
, m_clusterManager(new ClusterManager(m_comm, hosts))
, m_dataManager(new DataManager(m_comm))
, m_hubId(message::Id::Invalid)
, m_rank(r)
, m_size(hosts.size())
, m_recvSize(0)
, m_hubSocket(m_ioService)
{
   vassert(s_singleton == NULL);
   s_singleton = this;

   CERR << "started" << std::endl;

   message::DefaultSender::init(m_hubId, m_rank);

   // post requests for length of next MPI message
   if (m_size > 1) {
      MPI_Irecv(&m_recvSize, 1, MPI_INT, MPI_ANY_SOURCE, TagForBroadcast, MPI_COMM_WORLD, &m_reqAny);

      MPI_Irecv(m_recvBufToRank.data(), m_recvBufToRank.bufferSize(), MPI_BYTE, MPI_ANY_SOURCE, TagToRank, MPI_COMM_WORLD, &m_reqToRank);
   }
}

Communicator &Communicator::the() {

   vassert(s_singleton && "make sure to use the vistle Python module only from within vistle");
   if (!s_singleton)
      exit(1);
   return *s_singleton;
}

void Communicator::setModuleDir(const std::string &dir) {

    m_moduleDir = dir;
}

int Communicator::hubId() const {

   return m_hubId;
}

bool Communicator::isMaster() const {

   vassert(m_hubId <= Id::MasterHub); // make sure that hub id has already been set
   return m_hubId == Id::MasterHub;
}

void Communicator::setStatus(const std::string &text, int prio) {
    message::UpdateStatus t(text, (message::UpdateStatus::Importance)prio);
    sendMessage(message::Id::ForBroadcast, t);
}

void Communicator::clearStatus() {
    setStatus(std::string(), message::UpdateStatus::Bulk);
}

int Communicator::getRank() const {

   return m_rank;
}

int Communicator::getSize() const {

   return m_size;
}

bool Communicator::connectHub(std::string host, unsigned short port, unsigned short dataPort) {

   if (host == hostname())
       host = "localhost";
   if (getRank() == 0) {
       CERR << "connecting to hub on " << host << ":" << port << "..." << std::flush;
   }

   asio::ip::tcp::resolver resolver(m_ioService);
   asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
   auto ep = resolver.resolve(query);
   asio::ip::tcp::resolver::query queryd(host, boost::lexical_cast<std::string>(dataPort));
   m_dataEndpoint = resolver.resolve(queryd);
   boost::system::error_code ec;

   int ret = 1;
   if (getRank() == 0) {
      asio::connect(m_hubSocket, ep, ec);
      if (ec) {
         std::cerr << std::endl;
         CERR << "could not establish connection to hub at " << host << ":" << port << std::endl;
         ret = 0;
      } else {
         std::cerr << " ok." << std::endl;
      }
   }

   MPI_Bcast(&ret, 1, MPI_INT, 0, MPI_COMM_WORLD);

   return ret;
}

const AvailableMap &Communicator::localModules() {
    return m_localModules;
}

mpi::communicator Communicator::comm() const {

    return m_comm;
}

void Communicator::lock()
{
    m_mutex.lock();
}

void Communicator::unlock()
{
    m_mutex.unlock();
}

bool Communicator::connectData() {

    return m_dataManager->connect(m_dataEndpoint);
}

bool Communicator::sendHub(const message::Message &message) {

   if (getRank() == 0)
      return message::send(m_hubSocket, message);
   else
      return forwardToMaster(message);
}

bool Communicator::scanModules(const std::string &dir) {

   bool result = true;
#if defined(MODULE_THREAD) && defined(MODULE_STATIC)
   ModuleRegistry::the().availableModules(m_localModules);
#else
   if (m_localModules.empty()) {
       result = vistle::scanModules(dir, 0, m_localModules);
   }
#endif
   if (getRank() == 0) {
      for (auto &p: m_localModules) {
         const auto &m = p.second;
         message::ModuleAvailable msg(m_hubId, m.name, m.path);
         sendHub(msg);
      }
   }
   return result;
}

void Communicator::run() {

   bool work = false;
   while (dispatch(&work)) {

      if (parentProcessDied())
         throw(except::parent_died());

      vistle::adaptive_wait(work, this);
   }
   CERR << "Comm: run done" << std::endl;
}

bool Communicator::dispatch(bool *work) {

   bool done = false;

   bool received = false;

   // check for new UIs and other network clients
   // handle or broadcast messages received from slaves (rank > 0)
   if (m_size > 1) {
      int flag;
      MPI_Status status;
      MPI_Test(&m_reqToRank, &flag, &status);
      if (flag && status.MPI_TAG == TagToRank) {

         received = true;
         message::Message *message = &m_recvBufToRank;
         if (m_rank == 0 && message->isForBroadcast()) {
            if (!broadcastAndHandleMessage(*message)) {
               CERR << "Quit reason: broadcast & handle" << std::endl;
               done = true;
            }
         }  else {
            if (!handleMessage(*message)) {
               CERR << "Quit reason: handle" << std::endl;
               done = true;
            }
         }
         MPI_Irecv(m_recvBufToRank.data(), m_recvBufToRank.bufferSize(), MPI_BYTE, MPI_ANY_SOURCE, TagToRank, MPI_COMM_WORLD, &m_reqToRank);
      }

      // test for message size from another MPI node
      //    - receive actual message from broadcast (on any rank)
      //    - receive actual message from slave rank (on rank 0) for broadcasting
      //    - handle message
      //    - post another MPI receive for size of next message
      MPI_Test(&m_reqAny, &flag, &status);
      if (flag && status.MPI_TAG == TagForBroadcast) {

         if (m_recvSize > m_recvBufToAny.bufferSize()) {
            CERR << "invalid m_recvSize: " << m_recvSize << ", flag=" << flag << ", status.MPI_SOURCE=" << status.MPI_SOURCE << std::endl;
         }
         vassert(m_recvSize <= m_recvBufToAny.bufferSize());
         MPI_Bcast(m_recvBufToAny.data(), m_recvSize, MPI_BYTE,
               status.MPI_SOURCE, MPI_COMM_WORLD);
         if (m_recvSize > 0) {
            received = true;

            message::Message *message = &m_recvBufToAny;
#if 0
            printf("[%02d] message from [%02d] message type %d m_size %d\n",
                  m_rank, status.MPI_SOURCE, message->getType(), mpiMessageSize);
#endif
            if (!handleMessage(*message)) {
               CERR << "Quit reason: handle 2: " << *message << std::endl;
               done = true;
            }
         }

         MPI_Irecv(&m_recvSize, 1, MPI_INT, MPI_ANY_SOURCE, TagForBroadcast, MPI_COMM_WORLD, &m_reqAny);
      }

      for (auto it = m_ongoingSends.begin(), next = it; it != m_ongoingSends.end(); it = next) {
          ++next;
          MPI_Test(&(*it)->req, &flag, &status);
          if (flag) {
              m_ongoingSends.erase(it);
          }
      }
   }

   m_ioService.poll();
   if (m_rank == 0) {
      message::Buffer buf;
      bool gotMsg = false;
      if (!message::recv(m_hubSocket, buf, gotMsg)) {
         broadcastAndHandleMessage(message::Quit());
         CERR << "Quit reason: hub comm interrupted" << std::endl;
         done = true;
      } else if (gotMsg) {
         received = true;
         if (buf.destRank() == 0) {
             handleMessage(buf);
         } else if (buf.destRank() >= 0) {
             auto p = m_ongoingSends.emplace(new SendRequest(buf));
             auto it = p.first;
             MPI_Isend((*it)->buf.data(), (*it)->buf.size(), MPI_BYTE, buf.destRank(), TagToRank, MPI_COMM_WORLD, &(*it)->req);
         } else if(!broadcastAndHandleMessage(buf)) {
            CERR << "Quit reason: broadcast & handle 2: " << buf << buf << std::endl;
            done = true;
         }
      }
   }

   if (m_dataManager->dispatch())
       received = true;

   // test for messages from modules
   if (done) {
      CERR << "telling clusterManager to quit" << std::endl;
      if (m_clusterManager) {
         m_clusterManager->quit();
         done = false;
      }
   }
   if (m_clusterManager->quitOk()) {
      CERR << "Quit reason: clustermgr ready for quit" << std::endl;
      done = true;
   }
   if (!done && m_clusterManager) {
      done = !m_clusterManager->dispatch(received);
      if (done) {
         CERR << "Quit reason: clustermgr dispatch" << std::endl;
      }
   }
#if 0
   if (done) {
      delete m_clusterManager;
      m_clusterManager = nullptr;
   }
#endif

   if (work)
      *work = received;

   if (m_rank == 0 && done) {

       sendHub(message::Quit());
   }

   return !done;
}

bool Communicator::sendMessage(const int moduleId, const message::Message &message, int destRank) {

   if (m_rank == destRank || destRank == -1) {
      return clusterManager().sendMessage(moduleId, message);
   } else {
      auto p = m_ongoingSends.emplace(new SendRequest(message));
      auto it = p.first;
      MPI_Isend((*it)->buf.data(), (*it)->buf.size(), MPI_BYTE, destRank, TagToRank, MPI_COMM_WORLD, &(*it)->req);
   }
   return true;
}

bool Communicator::forwardToMaster(const message::Message &message) {

   vassert(m_rank != 0);
   if (m_rank != 0) {
      auto p = m_ongoingSends.emplace(new SendRequest(message));
      auto it = p.first;
      MPI_Isend(&(*it)->buf, (*it)->buf.size(), MPI_BYTE, 0, TagToRank, MPI_COMM_WORLD, &(*it)->req);
   }

   return true;
}

bool Communicator::broadcastAndHandleMessage(const message::Message &message) {

    assert(message.destRank() == -1);

    // message will be handled when received again from rank 0
    if (m_rank > 0)
        return forwardToMaster(message);

    if (m_size == 0)
        return handleMessage(message);

    message::Buffer buf(message);
    buf.setForBroadcast(false);
    buf.setWasBroadcast(true);

    std::vector<MPI_Request> s(m_size);
    for (int index = 0; index < m_size; ++index) {
        unsigned int size = buf.size();
        if (index != m_rank) {
            MPI_Isend(&size, 1, MPI_UNSIGNED, index, TagForBroadcast, MPI_COMM_WORLD, &s[index]);
        }
    }

    // wait for completion
    for (int index=0; index<m_size; ++index) {

        if (index == m_rank)
            continue;

        MPI_Wait(&s[index], MPI_STATUS_IGNORE);
    }

    MPI_Bcast(buf.data(), buf.size(), MPI_BYTE, m_rank, MPI_COMM_WORLD);

    const bool result = handleMessage(buf);

    return result;
}

bool Communicator::handleMessage(const message::Buffer &message) {

   std::lock_guard<Communicator> guard(*this);

   switch(message.type()) {
      case message::SETID: {
         auto &set = message.as<message::SetId>();
         m_hubId = set.getId();
         CERR << "got id " << m_hubId << std::endl;
         message::DefaultSender::init(m_hubId, m_rank);
         m_clusterManager->init();
         scanModules(m_moduleDir);
         return connectData();
         break;
      }
      case message::IDENTIFY: {
         auto &id = message.as<message::Identify>();
         CERR << "Identify message: " << id << std::endl;
         vassert(id.identity() == message::Identify::REQUEST);
         if (getRank() == 0)
             sendHub(message::Identify(message::Identify::MANAGER));
         scanModules(m_moduleDir);
         break;
      }
      default: {
         return m_clusterManager->handle(message);
      }
   }

   return true;
}

Communicator::~Communicator() {

   delete m_dataManager;
   m_dataManager = nullptr;

   CERR << "shut down: deleting clusterManager" << std::endl;
   delete m_clusterManager;
   m_clusterManager = NULL;

   CERR << "shut down: start init barrier" << std::endl;
   MPI_Barrier(MPI_COMM_WORLD);
   CERR << "shut down: done init BARRIER" << std::endl;

   if (m_size > 1) {
      int dummy = 0;
      MPI_Request s;
      MPI_Isend(&dummy, 1, MPI_INT, (m_rank + 1) % m_size, TagForBroadcast, MPI_COMM_WORLD, &s);
      MPI_Wait(&s, MPI_STATUS_IGNORE);
      CERR << "wait for sending to any" << std::endl;
      MPI_Wait(&m_reqAny, MPI_STATUS_IGNORE);
      CERR << "wait for receiving, to any" << std::endl;

#if 0
      if (m_rank == 1) {
         MPI_Request s2;
         MPI_Isend(&dummy, 1, MPI_BYTE, 0, TagToRank, MPI_COMM_WORLD, &s2);
         MPI_Wait(&s2, MPI_STATUS_IGNORE);
         CERR << "wait for send to 0" << std::endl;
      }
      if (m_rank == 0) {
         MPI_Wait(&m_reqToRank, MPI_STATUS_IGNORE);
         CERR << "wait for recv from 1" << std::endl;
      }
#endif
   }
   CERR << "SHUT DOWN COMPLETE" << std::endl;
   MPI_Barrier(MPI_COMM_WORLD);
   CERR << "SHUT DOWN BARRIER COMPLETE" << std::endl;
}

ClusterManager &Communicator::clusterManager() const {

   return *m_clusterManager;
}

DataManager &Communicator::dataManager() const {

   return *m_dataManager;
}

} // namespace vistle
