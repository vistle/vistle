#ifndef COMMUNICATOR_COLLECTIVE_H
#define COMMUNICATOR_COLLECTIVE_H

#include <vector>

#include <boost/asio.hpp>

#include <mpi.h>

#include <core/message.h>

namespace vistle {

class Parameter;
class PythonEmbed;
class ClusterManager;
class DataManager;

class Communicator {
   friend class ClusterManager;
   friend class DataManager;

 public:
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
   bool sendMessage(int receiver, const message::Message &message, int rank=-1) const;
   void setQuitFlag();

   int hubId() const;
   int getRank() const;
   int getSize() const;

   unsigned short uiPort() const;

   ClusterManager &clusterManager() const;
   DataManager &dataManager() const;
   bool connectHub(const std::string &host, unsigned short port, unsigned short dataPort);

 private:
   bool sendHub(const message::Message &message);
   bool connectData();
   bool scanModules(const std::string &dir);


   ClusterManager *m_clusterManager;
   DataManager *m_dataManager;

   bool isMaster() const;
   int m_hubId;
   const int m_rank;
   const int m_size;
   std::string m_moduleDir;

   bool m_quitFlag;

   unsigned m_recvSize;
   message::Buffer m_recvBufToRank, m_recvBufToAny;
   MPI_Request m_reqAny, m_reqToRank;

   static Communicator *s_singleton;

   Communicator(const Communicator &other); // not implemented

   boost::asio::io_service m_ioService;
   boost::asio::ip::tcp::socket m_hubSocket;
   boost::asio::ip::tcp::resolver::iterator m_dataEndpoint;
};

} // namespace vistle

#endif
