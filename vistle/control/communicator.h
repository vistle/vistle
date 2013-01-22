#ifndef COMMUNICATOR_COLLECTIVE_H
#define COMMUNICATOR_COLLECTIVE_H

#include <vector>
#include <deque>
#include <map>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/asio.hpp>

#include <mpi.h>

#include "portmanager.h"
#include "client.h"

namespace bi = boost::interprocess;

namespace vistle {

namespace message {
   struct Message;
   class MessageQueue;
}

class Parameter;

class PythonEmbed;

class Communicator {
   friend class PythonEmbed;
   friend class InteractiveClient;

 public:
   Communicator(int argc, char *argv[], int rank, int size);
   ~Communicator();
   static Communicator &the();
   InteractiveClient *activeClient() const;

   void registerInterpreter(PythonEmbed *pi);
   void setInput(const std::string &input);
   void setFile(const std::string &filename);

   bool dispatch();
   bool handleMessage(const message::Message &message);
   bool broadcastAndHandleMessage(const message::Message &message);
   void setQuitFlag();

   int getRank() const;
   int getSize() const;

   void resetModuleCounter();
   int newModuleID();
   int getBarrierCounter();
   boost::mutex &barrierMutex();
   boost::condition_variable &barrierCondition();
   std::vector<int> getRunningList() const;
   std::vector<int> getBusyList() const;
   std::string getModuleName(int id) const;

   std::vector<std::string> getParameters(int id) const;
   const Parameter *getParameter(int id, const std::string &name) const;

   const PortManager &portManager() const;

   void acquireInterpreter(InteractiveClient *client);
   void releaseInterpreter();
   bool execute(const std::string &line);

 private:

   std::string m_bindir;

   std::string m_initialFile;
   std::string m_initialInput;

   const int rank;
   const int size;

   bool m_quitFlag;

   boost::asio::io_service m_ioService;
   boost::asio::ip::tcp::acceptor m_acceptor;

   boost::mutex m_barrierMutex;
   boost::condition_variable m_barrierCondition;
   int moduleID;
   PythonEmbed *interpreter;
   boost::mutex interpreter_mutex;
   void barrierReached(int id);

   char *mpiReceiveBuffer;
   int mpiMessageSize;

   MPI_Request request;

   typedef std::map<int, message::MessageQueue *> MessageQueueMap;
   MessageQueueMap sendMessageQueue;
   MessageQueueMap receiveMessageQueue;
   boost::interprocess::message_queue messageQueue;
   bool tryReceiveAndHandleMessage(boost::interprocess::message_queue &mq, bool &received, bool broadcast=false);

   std::map<int, bi::shared_memory_object *> shmObjects;

   typedef std::map<int, std::string> RunningMap;
   RunningMap runningMap;
   typedef std::set<int> ModuleSet;
   ModuleSet busySet;

   typedef std::map<std::string, Parameter *> ModuleParameterMap;
   typedef std::map<int, ModuleParameterMap> ParameterMap;
   ParameterMap parameterMap;

   PortManager m_portManager;
   int m_moduleCounter;
   int m_barrierCounter;
   int m_activeBarrier;
   int m_reachedBarriers;
   ModuleSet reachedSet;

   InteractiveClient *m_activeClient;
   ReadlineClient m_console;
   boost::thread m_consoleThread;
   bool m_consoleThreadCreated;
   int acceptClients();
   void disconnectClients();
   void startAccept();
   void handleAccept(AsioClient &client, const boost::system::error_code &error);
   unsigned short m_port;

   static Communicator *s_singleton;
};

} // namespace vistle

#endif
