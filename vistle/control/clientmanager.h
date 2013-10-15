#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <map>

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include "export.h"

namespace vistle {

class Client;
class ReadlineClient;
class AsioClient;
class PythonInterface;
class PythonModule;

class V_CONTROLEXPORT ClientManager {
   friend class ClientThreadWrapper;
   friend class Communicator;

 public:
   ~ClientManager();

   Client *activeClient() const;
   void requestQuit();
   boost::mutex &interpreterMutex();

   bool execute(const std::string &line, Client *client=NULL);
   bool executeFile(const std::string &filename, Client *client=NULL);

 private:
   enum InitialType {
      File, String
   };

   ClientManager(const std::string &initialData=std::string(),
         InitialType initialType=File,
         unsigned short port=31092);

   bool check();
   unsigned short port() const;

   void addClient(Client *c);
   void removeThread(boost::thread *thread);

   void startServer();
   void join();
   void disconnect();

   void startAccept();
   void handleAccept(AsioClient *client, const boost::system::error_code &error);

   PythonInterface *interpreter;
   PythonModule *m_module;
   boost::mutex interpreter_mutex;
   unsigned short m_port;

   bool m_requestQuit;
   boost::asio::io_service m_ioService;
   boost::asio::ip::tcp::acceptor m_acceptor;

   Client *m_activeClient;
   ReadlineClient *m_console;
   typedef std::map<boost::thread *, Client *> ThreadMap;
   ThreadMap m_threads;
};

} // namespace vistle

#endif
