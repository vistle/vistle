#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

#include <core/message.h>

namespace vistle {

class UiClient;
class StateTracker;
class Hub;

class UiManager {

 public:
   UiManager(Hub &hub, StateTracker &stateTracker);
   ~UiManager();

   void requestQuit();
   bool handleMessage(const message::Message &msg, boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void sendMessage(const message::Message &msg);
   void addClient(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void lockUi(bool lock);
   bool isLocked() const;

 private:
   bool sendMessage(boost::shared_ptr<UiClient> c, const message::Message &msg) const;

   void disconnect();
   bool removeClient(boost::shared_ptr<UiClient> c);

   Hub &m_hub;
   StateTracker &m_stateTracker;

   bool m_locked;
   bool m_requestQuit;

   int m_uiCount;
   std::map<boost::shared_ptr<boost::asio::ip::tcp::socket>, boost::shared_ptr<UiClient>> m_clients;
   std::vector<message::Buffer> m_queue;
};

} // namespace vistle

#endif
