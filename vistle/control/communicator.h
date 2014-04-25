#ifndef COMMUNICATOR_COLLECTIVE_H
#define COMMUNICATOR_COLLECTIVE_H

#include <vector>

#include <boost/asio.hpp>

#include <mpi.h>

#include <core/message.h>

#include "export.h"

namespace vistle {

class Parameter;
class PythonEmbed;
class ModuleManager;

class V_CONTROLEXPORT Communicator {
   friend class ModuleManager;

 public:
   Communicator(int argc, char *argv[], int rank, const std::vector<std::string> &hosts);
   ~Communicator();
   static Communicator &the();

   bool scanModules(const std::string &dir);

   bool dispatch();
   bool handleMessage(const message::Message &message);
   bool forwardToMaster(const message::Message &message);
   bool broadcastAndHandleMessage(const message::Message &message);
   bool sendMessage(int receiver, const message::Message &message) const;
   void setQuitFlag();

   int getRank() const;
   int getSize() const;

   unsigned short uiPort() const;

   ModuleManager &moduleManager() const;
   bool connectHub(const std::string &host, unsigned short port);

 private:
   bool sendHub(const message::Message &message);

   ModuleManager *m_moduleManager;

   const int m_rank;
   const int m_size;

   bool m_quitFlag;

   int m_recvSize;
   std::vector<char> m_recvBufTo0, m_recvBufToAny;
   MPI_Request m_reqAny, m_reqToRank0;

   int m_traceMessages;

   static Communicator *s_singleton;

   Communicator(const Communicator &other); // not implemented

   boost::asio::io_service m_ioService;
   boost::asio::ip::tcp::socket m_hubSocket;
};

} // namespace vistle

#endif
