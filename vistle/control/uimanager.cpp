/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <boost/asio.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include "pythonembed.h"
#include "uimanager.h"
#include "uiclient.h"
#include "communicator.h"
#include "modulemanager.h"

#include <core/messagequeue.h>

namespace asio = boost::asio;

namespace vistle {

class UiThreadWrapper {

 public:
   UiThreadWrapper(UiClient *c, UiManager *m)
   : release(true)
   , manager(m)
   , thread(NULL)
   , client(c)
   {} 

   UiThreadWrapper(const UiThreadWrapper &o)
   : release(o.release)
   , manager(o.manager)
   , thread(o.thread)
   , client(o.client)
   {
      o.release = false;
   }

   ~UiThreadWrapper() {
      if (release) {
         manager->removeThread(thread);
         delete client;
      }
   }

   void operator()() {
      (*client)();

      assert(client->done());
   }

   private:
   mutable bool release;
   UiManager *manager;
   boost::thread *thread;
   UiClient *client;
};

UiManager::UiManager(boost::shared_ptr<message::MessageQueue> commandQueue, unsigned short port)
: m_commandQueue(commandQueue)
, m_port(port)
, m_requestQuit(false)
, m_acceptor(m_ioService)
{

   startServer();

   check();
}

unsigned short UiManager::port() const {

   return m_port;
}

void UiManager::sendMessage(const message::Message &msg) const {

   for(auto ent: m_threads) {
      sendMessage(ent.second, msg);
   }
}

void UiManager::sendMessage(UiClient *c, const message::Message &msg) const {

   boost::interprocess::message_queue &mq = c->recvQueue();
   mq.send(&msg, msg.size(), 0);
}

void UiManager::requestQuit() {

   m_requestQuit = true;
}

UiManager::~UiManager() {

   disconnect();
}

void UiManager::removeThread(boost::thread *thread) {

   m_threads.erase(thread);
}

void UiManager::disconnect() {

   BOOST_FOREACH(ThreadMap::value_type v, m_threads) {
      UiClient *c = v.second;
      c->cancel();
   }

   join();
   if (!m_threads.empty()) {
      std::cerr << "UiManager: waiting for " << m_threads.size() << " threads to quit" << std::endl;
      sleep(1);
      join();
   }

}

void UiManager::startAccept() {

   ++m_uiCount;
   boost::shared_ptr<message::MessageQueue> smq = m_commandQueue;
   boost::shared_ptr<message::MessageQueue> rmq(message::MessageQueue::create(message::MessageQueue::createName("rui", m_uiCount, 0)));
   UiClient *client = new UiClient(*this, -m_uiCount, smq, rmq);
   m_acceptor.async_accept(client->socket(), boost::bind<void>(&UiManager::handleAccept, this, client, asio::placeholders::error));
}

void UiManager::handleAccept(UiClient *client, const boost::system::error_code &error) {

   if (!error) {
      addClient(client);
   }

   startAccept();
}

void UiManager::addClient(UiClient *c) {

   boost::thread *t = new boost::thread(UiThreadWrapper(c, this));
   m_threads[t] = c;

   sendMessage(c, message::SetId(c->id()));

   std::vector<char> state = Communicator::the().moduleManager().getState();

   for (size_t i=0; i<state.size(); i+=message::Message::MESSAGE_SIZE) {
      const message::Message *msg = (const message::Message *)&state[i];
      sendMessage(c, *msg);
   }
   sendMessage(c, message::ReplayFinished());
}

void UiManager::join() {

   for (ThreadMap::iterator it = m_threads.begin();
         it != m_threads.end();
       ) {
      boost::thread *t = it->first;
      UiClient *c = it->second;
      ThreadMap::iterator next = it;
      ++next;
      if (c->done()) {
         t->join();
         m_threads.erase(it);
      }
      it = next;
   }
}

void UiManager::startServer() {

   while (!m_acceptor.is_open()) {
 
      asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v6(), m_port);
      m_acceptor.open(endpoint.protocol());
      m_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
      try {
         m_acceptor.bind(endpoint);
      } catch(const boost::system::system_error &err) {
         if (err.code() == boost::system::errc::address_in_use) {
            m_acceptor.close();
            ++m_port;
            continue;
         }
         throw(err);
      }
      m_acceptor.listen();
      std::cerr << "Listening for UIs on port " << m_port << std::endl;
      startAccept();
   }

   m_ioService.poll();
}

bool UiManager::check() {

   m_ioService.poll();

   join();

   return !m_requestQuit;
}

} // namespace vistle
