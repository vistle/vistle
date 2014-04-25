#ifndef VISTLE_HUB_H
#define VISTLE_HUB_H

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <core/porttracker.h>
#include <core/statetracker.h>
#include <util/spawnprocess.h>
#include "uimanager.h"

namespace vistle {

class Hub {

 public:
   typedef boost::asio::ip::tcp::socket socket;
   
   static Hub &the();

   Hub();
   ~Hub();

   bool init(int argc, char *argv[]);
   bool processScript();
   bool startUi();
   bool dispatch();
   bool sendMessage(boost::shared_ptr<socket> sock, const message::Message &msg);
   unsigned short port() const;

   bool handleMessage(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, const message::Message &msg);
   bool handleUiMessage(const message::Message &msg);

   bool sendMaster(const message::Message &msg);
   bool sendSlaves(const message::Message &msg);
   bool sendUi(const message::Message &msg);

   const StateTracker &stateTracker() const;
   StateTracker &stateTracker();

private:
   bool startServer();
   bool startAccept();
   void handleAccept(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code &error);
   void addSocket(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
   bool removeSocket(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void addClient(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);

   unsigned short m_port;
   boost::asio::io_service m_ioService;
   boost::asio::ip::tcp::acceptor m_acceptor;

   std::map<boost::shared_ptr<boost::asio::ip::tcp::socket>, message::Identify::Identity> m_sockets;
   std::set<boost::shared_ptr<boost::asio::ip::tcp::socket>> m_clients;

   PortTracker m_portTracker;
   StateTracker m_stateTracker;
   UiManager m_uiManager;
   int m_idCount;

   std::map<process_handle, int> m_processMap;
   bool m_managerConnected;

   std::string m_uiPath;
   std::string m_scriptPath;
   bool m_quitting;
};

}
#endif
