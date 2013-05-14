#ifndef COMMUNICATOR_COLLECTIVE_H
#define COMMUNICATOR_COLLECTIVE_H

#include <vector>
#include <map>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

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
class ClientManager;

class Communicator {
   friend class PythonEmbed;

 public:
   Communicator(int argc, char *argv[], int rank, int size);
   ~Communicator();
   static Communicator &the();

   void setInput(const std::string &input);
   void setFile(const std::string &filename);

   bool dispatch();
   bool handleMessage(const message::Message &message);
   bool broadcastAndHandleMessage(const message::Message &message);
   bool sendMessage(int receiver, const message::Message &message) const;
   void setQuitFlag();

   int getRank() const;
   int getSize() const;

   void resetModuleCounter();
   int newModuleID();
   int newExecutionCount();
   int getBarrierCounter();
   boost::mutex &barrierMutex();
   boost::condition_variable &barrierCondition();
   std::vector<int> getRunningList() const;
   std::vector<int> getBusyList() const;
   std::string getModuleName(int id) const;

   std::vector<std::string> getParameters(int id) const;
   const Parameter *getParameter(int id, const std::string &name) const;

   const PortManager &portManager() const;

 private:
   ClientManager *m_clientManager;

   std::string m_bindir;

   std::string m_initialFile;
   std::string m_initialInput;

   const int rank;
   const int size;

   bool m_quitFlag;

   boost::mutex m_barrierMutex;
   boost::condition_variable m_barrierCondition;
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
   int m_executionCounter;
   int m_barrierCounter;
   int m_activeBarrier;
   int m_reachedBarriers;
   ModuleSet reachedSet;

   static Communicator *s_singleton;
};

} // namespace vistle

#endif
