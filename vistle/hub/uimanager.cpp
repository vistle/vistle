/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <boost/thread.hpp>

#include <core/statetracker.h>
#include <core/messagequeue.h>
#include <core/tcpmessage.h>

#include "uimanager.h"
#include "uiclient.h"
#include "hub.h"

namespace vistle {

UiManager::UiManager(Hub &hub, StateTracker &stateTracker)
: m_hub(hub)
, m_stateTracker(stateTracker)
, m_requestQuit(false)
, m_uiCount(0)
, m_locked(false)
{
}

void UiManager::sendMessage(const message::Message &msg) const {

   for(auto ent: m_threads) {
      sendMessage(ent.second, msg);
   }
}

void UiManager::sendMessage(boost::shared_ptr<UiClient> c, const message::Message &msg) const {

   auto &ioService = c->socket().get_io_service();
   message::send(c->socket(), msg);
   ioService.poll();
}

bool UiManager::handleMessage(const message::Message &msg) const {

   return m_hub.handleUiMessage(msg);
}

void UiManager::requestQuit() {

   m_requestQuit = true;
   sendMessage(message::Quit());
}

UiManager::~UiManager() {

   disconnect();
}

void UiManager::removeThread(boost::thread *thread) {

   m_threads.erase(thread);
}

void UiManager::disconnect() {

   sendMessage(message::Quit());

   for(const auto &v: m_threads) {
      const auto &c = v.second;
      c->cancel();
   }

   join();
   if (!m_threads.empty()) {
      std::cerr << "UiManager: waiting for " << m_threads.size() << " threads to quit" << std::endl;
      sleep(1);
      join();
   }

}

void UiManager::addClient(boost::shared_ptr<UiClient> c) {

   boost::thread *t = new boost::thread(boost::ref(*c));
   m_threads[t] = c;

   if (m_requestQuit) {

      sendMessage(c, message::Quit());
   } else {

      sendMessage(c, message::SetId(c->id()));

      sendMessage(c, message::LockUi(m_locked));

      std::vector<char> state = m_stateTracker.getState();

      for (size_t i=0; i<state.size(); i+=message::Message::MESSAGE_SIZE) {
         const message::Message *msg = (const message::Message *)&state[i];
         sendMessage(c, *msg);
      }
      sendMessage(c, message::ReplayFinished());

      auto avail = m_stateTracker.availableModules();
      for(const auto &mod: avail) {
         sendMessage(c, message::ModuleAvailable(mod.name));
      }
   }
}

void UiManager::join() {

   for (ThreadMap::iterator it = m_threads.begin();
         it != m_threads.end();
       ) {
      boost::thread *t = it->first;
      auto c = it->second;
      ThreadMap::iterator next = it;
      ++next;
      if (c->done()) {
         t->join();
         m_threads.erase(it);
      }
      it = next;
   }
}

void UiManager::lockUi(bool locked) {

   if (m_locked != locked) {
      sendMessage(message::LockUi(locked));
      m_locked = locked;
   }
}

} // namespace vistle
