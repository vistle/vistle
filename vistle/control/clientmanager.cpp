/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <boost/asio.hpp>

#include "pythonembed.h"
#include "clientmanager.h"
#include "client.h"

namespace asio = boost::asio;

namespace vistle {

class ClientThreadWrapper {

   public:
   ClientThreadWrapper(Client *c, ClientManager *m)
   : release(true)
   , manager(m)
   , thread(NULL)
   , client(c)
   {}

   ClientThreadWrapper(const ClientThreadWrapper &o)
   : release(o.release)
   , manager(o.manager)
   , thread(o.thread)
   , client(o.client)
   {
      o.release = false;
   }

   ~ClientThreadWrapper() {
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
   ClientManager *manager;
   boost::thread *thread;
   Client *client;
};

ClientManager::ClientManager(const std::string &initial, InitialType initialType, unsigned short port)
: interpreter(new PythonEmbed(*this, "vistle"))
, m_port(port)
, m_requestQuit(false)
, m_acceptor(m_ioService)
, m_activeClient(NULL)
, m_console(NULL)
{

   if (!initial.empty()) {
      if (initialType == File) {
         addClient(new FileClient(*this, initial));
      } else {
         addClient(new BufferClient(*this, initial));
      }
   }

   startServer();

   m_console = new ReadlineClient(*this);
   addClient(m_console);

   check();
}

unsigned short ClientManager::port() const {

   return m_port;
}

Client *ClientManager::activeClient() const {

   return m_activeClient;
}

void ClientManager::requestQuit() {

   m_requestQuit = true;
}

boost::mutex &ClientManager::interpreterMutex() {

   return interpreter_mutex;
}

bool ClientManager::execute(const std::string &line, Client *client) {

   m_activeClient = client;
   bool ret = interpreter->exec(line);
   m_activeClient = NULL;
   return ret;
}

bool ClientManager::executeFile(const std::string &filename, Client *client) {

   m_activeClient = client;
   bool ret = interpreter->exec_file(filename);
   m_activeClient = NULL;
   return ret;
}

ClientManager::~ClientManager() {

   disconnect();
}

void ClientManager::removeThread(boost::thread *thread) {

   m_threads.erase(thread);
}

void ClientManager::disconnect() {

   BOOST_FOREACH(ThreadMap::value_type v, m_threads) {
      Client *c = v.second;
      c->cancel();
   }

   join();
   if (!m_threads.empty()) {
      std::cerr << "waiting for " << m_threads.size() << " threads to quit" << std::endl;
      sleep(1);
      join();
   }

}

void ClientManager::startAccept() {

      AsioClient *client = new AsioClient(*this);
      m_acceptor.async_accept(client->socket(), boost::bind<void>(&ClientManager::handleAccept, this, client, asio::placeholders::error));
}

void ClientManager::handleAccept(AsioClient *client, const boost::system::error_code &error) {

   if (!error) {
      addClient(client);
   }

   startAccept();
}

void ClientManager::addClient(Client *c) {

   boost::thread *t = new boost::thread(ClientThreadWrapper(c, this));
   m_threads[t] = c;
}

void ClientManager::join() {

   for (ThreadMap::iterator it = m_threads.begin();
         it != m_threads.end();
       ) {
      boost::thread *t = it->first;
      Client *c = it->second;
      ThreadMap::iterator next = it;
      ++next;
      if (c->done()) {
         t->join();
         m_threads.erase(it);
      }
      it = next;
   }
}

void ClientManager::startServer() {

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
      std::cerr << "Listening for your commands on port " << m_port << std::endl;
      startAccept();
   }

   m_ioService.poll();
}

bool ClientManager::check() {

   m_ioService.poll();

   join();

   return !m_requestQuit;
}

} // namespace vistle
