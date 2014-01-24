#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <map>

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/shared_ptr.hpp>

#include <core/messagequeue.h>

#include "export.h"

namespace vistle {

namespace message {
class Message;
};

class UiClient;

class V_CONTROLEXPORT UiManager {
   friend class UiThreadWrapper;
   friend class Communicator;

 public:
   ~UiManager();

   void requestQuit();
   boost::mutex &interpreterMutex();
   void sendMessage(const message::Message &msg) const;

 private:
   UiManager(boost::shared_ptr<message::MessageQueue> commandQueue, unsigned short port=31093);

   bool check();
   unsigned short port() const;

   void addClient(boost::shared_ptr<UiClient> c);
   void removeThread(boost::thread *thread);
   void sendMessage(boost::shared_ptr<UiClient> c, const message::Message &msg) const;

   void startServer();
   void join();
   void disconnect();

   void startAccept();
   void handleAccept(boost::shared_ptr<UiClient> client, const boost::system::error_code &error);

   boost::shared_ptr<message::MessageQueue> m_commandQueue;
   unsigned short m_port;

   bool m_requestQuit;
   boost::asio::io_service m_ioService;
   boost::asio::ip::tcp::acceptor m_acceptor;
   boost::shared_ptr<UiClient> m_listeningClient;

   typedef std::map<boost::thread *, boost::shared_ptr<UiClient>> ThreadMap;
   ThreadMap m_threads;
   int m_uiCount;
};

} // namespace vistle

#endif
