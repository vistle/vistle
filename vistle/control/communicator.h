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

#include <mpi.h>

#include "portmanager.h"

namespace bi = boost::interprocess;

namespace vistle {

namespace message {
   struct Message;
   class MessageQueue;
}

class Parameter;

class PythonEmbed;

class InteractiveClient {
   friend class PythonEmbed;

   public:
      InteractiveClient();
      InteractiveClient(int readfd, int writefd=-1 /* same as readfd */, bool keepInterpreterLock=false);
      InteractiveClient(const InteractiveClient &o);
      ~InteractiveClient();

      void operator()();

      void setQuitOnEOF();
      void setInput(const std::string &input);

      bool write(const std::string &s);

   private:
      mutable bool m_close;
      bool readline(std::string &line);
      bool printPrompt();
      bool printGreeting();
      int readfd, writefd;
      std::vector<char> readbuf;
      bool m_quitOnEOF;
      bool m_keepInterpreter;
      bool m_useReadline;
};

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

   boost::mutex m_barrierMutex;
   boost::condition_variable m_barrierCondition;
   int serverSocket;
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
   InteractiveClient m_console;
   boost::thread m_consoleThread;
   bool m_consoleThreadCreated;
   int acceptClients();
   bool setClientBlocking(int num, bool block);
   void allocateBuffers(int num);
   void flushClient(int num);
   void writeClient(int num, const void *buf, size_t n);
   void writeClient(int num, const std::string &s);
   void printGreeting(int num);
   void printPrompt(int num);
   ssize_t readClient(int num, void *buf, size_t n);
   bool hasClientLine(int num);
   std::string readClientLine(int num);
   ssize_t fillClientBuffer(int num);
   void disconnectClients();
   unsigned short m_port;

   static Communicator *s_singleton;

   static const int FdOut = 1;
   enum { StdInOut = 0, Server = 1, SocketStart = 2 };
};

} // namespace vistle

#endif
