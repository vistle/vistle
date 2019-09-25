/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <core/statetracker.h>
#include <core/messagequeue.h>
#include <core/tcpmessage.h>

#include "uimanager.h"
#include "uiclient.h"
#include "hub.h"

#include <boost/version.hpp>

namespace vistle {

UiManager::UiManager(Hub &hub, StateTracker &stateTracker)
: m_hub(hub)
, m_stateTracker(stateTracker)
, m_locked(false)
, m_requestQuit(false)
, m_uiCount(vistle::message::Id::ModuleBase)
{
}

UiManager::~UiManager() {

   disconnect();
}

bool UiManager::handleMessage(std::shared_ptr<boost::asio::ip::tcp::socket> sock, const message::Message &msg, const std::vector<char> &payload) {

   using namespace vistle::message;

   std::shared_ptr<UiClient> sender;
    auto it = m_clients.find(sock);
    if (it == m_clients.end()) {
        std::cerr << "UiManager: message from unknown UI" << std::endl;
    } else {
        sender = it->second;
    }

   switch(msg.type()) {
   case MODULEEXIT:
   case QUIT: {
       if (!sender) {
           std::cerr << "UiManager: unknown UI quit" << std::endl;
       } else {
           sender->cancel();
           removeClient(sender);
       }
       if (msg.type() == MODULEEXIT)
           return true;
       break;
   }
   default:
       break;
   }

   if (isLocked()) {

      m_queue.emplace_back(msg);
      return true;
   }

   return m_hub.handleMessage(msg, sock, &payload);
}

void UiManager::sendMessage(const message::Message &msg, int id, const std::vector<char> *payload) const {

   std::vector<std::shared_ptr<UiClient>> toRemove;

   for(auto ent: m_clients) {
       if (id==message::Id::Broadcast || ent.second->id()==id) {
           if (!sendMessage(ent.second, msg, payload)) {
               toRemove.push_back(ent.second);
           }
       }
   }

   for (auto ent: toRemove) {
      removeClient(ent);
   }
}

bool UiManager::sendMessage(std::shared_ptr<UiClient> c, const message::Message &msg, const std::vector<char> *payload) const {

#if BOOST_VERSION < 107000
   //FIXME is message reliably sent, e.g. also during shutdown, without polling?
   auto &ioService = c->socket()->get_io_service();
#endif
   bool ret = message::send(*c->socket(), msg, payload);
#if BOOST_VERSION < 107000
   ioService.poll();
#endif
   return ret;
}

void UiManager::requestQuit() {

   m_requestQuit = true;
   sendMessage(message::Quit());
}

void UiManager::disconnect() {

   sendMessage(message::Quit());

   for(const auto &c: m_clients) {
      c.second->cancel();
   }

   m_clients.clear();
}

void UiManager::addClient(std::shared_ptr<boost::asio::ip::tcp::socket> sock) {

   std::shared_ptr<UiClient> c(new UiClient(*this, m_uiCount, sock));

   m_clients.insert(std::make_pair(sock, c));

   std::cerr << "UiManager: new UI " << m_uiCount << " connected, now have " << m_clients.size() << " connections" << std::endl;
   ++m_uiCount;

   if (m_requestQuit) {

      sendMessage(c, message::Quit());
   } else {

      sendMessage(c, message::SetId(c->id()));

      sendMessage(c, message::LockUi(m_locked));

      auto state = m_stateTracker.getState();
      for (auto &m: state) {
          sendMessage(c, m.message, m.payload.get());
      }
   }
}

bool UiManager::removeClient(std::shared_ptr<UiClient> c) const {

   std::cerr << "UiManager: removing client " << c->id() << std::endl;

   for (auto &ent: m_clients) {
      if (ent.second == c) {
         if (!c->done()) {
             sendMessage(c, message::Quit());
             c->cancel();
         }
         m_clients.erase(ent.first);
         return true;
      }
   }

   return false;
}

void UiManager::lockUi(bool locked) {

   if (m_locked != locked) {
      sendMessage(message::LockUi(locked));
      m_locked = locked;
   }

   if (!m_locked) {
      for (auto &m: m_queue) {
         m_hub.handleMessage(m);
      }
      m_queue.clear();
   }
}

bool UiManager::isLocked() const {

   return m_locked;
}

} // namespace vistle
