#ifndef COMMUNICATOR_COLLECTIVE_H
#define COMMUNICATOR_COLLECTIVE_H

#include <vector>
#include <set>

#include <boost/asio.hpp>
#include <boost/mpi.hpp>

#include <core/message.h>
#include <util/directory.h>

namespace vistle {

class Parameter;
class PythonEmbed;
class ClusterManager;
class DataManager;

class Communicator {
   friend class ClusterManager;
   friend class DataManager;

 public:

   enum MpiTags {
       TagToRank,
       TagForBroadcast,
       TagData,
   };

   Communicator(int rank, const std::vector<std::string> &hosts);
   ~Communicator();
   static Communicator &the();

   void setModuleDir(const std::string &dir);
   void run();
   bool dispatch(bool *work);
   bool handleMessage(const message::Buffer &message);
   bool handleDataMessage(const message::Message &message);
   bool forwardToMaster(const message::Message &message);
   bool broadcastAndHandleMessage(const message::Message &message);
   bool sendMessage(int receiver, const message::Message &message, int rank=-1);

   int hubId() const;
   int getRank() const;
   int getSize() const;

   unsigned short uiPort() const;

   ClusterManager &clusterManager() const;
   DataManager &dataManager() const;
   bool connectHub(std::string host, unsigned short port, unsigned short dataPort);
   const AvailableMap &localModules();
   boost::mpi::communicator comm() const;

   void lock();
   void unlock();

 private:
   bool sendHub(const message::Message &message);
   bool connectData();
   bool scanModules(const std::string &dir);


   boost::mpi::communicator m_comm;
   ClusterManager *m_clusterManager;
   DataManager *m_dataManager;

   std::recursive_mutex m_mutex;
   bool isMaster() const;
   int m_hubId;
   const int m_rank;
   const int m_size;
   std::string m_moduleDir;

   unsigned m_recvSize;
   message::Buffer m_recvBufToRank, m_recvBufToAny;
   MPI_Request m_reqAny, m_reqToRank;
   struct SendRequest {
       SendRequest(const message::Message &msg): buf(msg) {}
       SendRequest(const message::Buffer &buf): buf(buf) {}
       MPI_Request req;
       message::Buffer buf;
   };
   std::set<std::shared_ptr<SendRequest>> m_ongoingSends;

   static Communicator *s_singleton;

   Communicator(const Communicator &other); // not implemented

   boost::asio::io_service m_ioService;
   boost::asio::ip::tcp::socket m_hubSocket;
   boost::asio::ip::tcp::resolver::iterator m_dataEndpoint;

   AvailableMap m_localModules;

   void setStatus(const std::string &text, int prio);
   void clearStatus();
};

} // namespace vistle

#endif
