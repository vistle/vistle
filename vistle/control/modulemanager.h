#ifndef MODULEMANAGER_H
#define MODULEMANAGER_H

#include <vector>
#include <map>

#include <boost/interprocess/ipc/message_queue.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include "portmanager.h"
#include <core/statetracker.h>
#include <core/message.h>
#include <core/messagequeue.h>

namespace vistle {

namespace message {
   class Message;
   class MessageQueue;
}

class Parameter;

class ModuleManager {
   friend class Communicator;

 public:
   ModuleManager(int argc, char *argv[], int rank, const std::vector<std::string> &hosts);
   ~ModuleManager();
   static ModuleManager &the();

   bool dispatch(bool &received);

   bool sendMessage(int receiver, const message::Message &message) const;
   bool sendAll(const message::Message &message) const;
   bool sendAllOthers(int excluded, const message::Message &message) const;
   bool sendUi(const message::Message &message) const;

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
   Parameter *getParameter(int id, const std::string &name) const;

   std::vector<char> getState() const;

   const PortManager &portManager() const;

   bool checkMessageQueue() const;

 private:
   void queueMessage(const message::Message &msg);
   void replayMessages();
   std::vector<char> m_messageQueue;

   PortManager m_portManager;
   StateTracker m_stateTracker;

   // only used by Communicator
   bool handle(const message::Ping &ping);
   bool handle(const message::Pong &pong);
   bool handle(const message::Spawn &spawn);
   bool handle(const message::Started &started);
   bool handle(const message::Connect &connect);
   bool handle(const message::Disconnect &disc);
   bool handle(const message::ModuleExit &moduleExit);
   bool handle(const message::Compute &compute);
   bool handle(const message::Busy &busy);
   bool handle(const message::Idle &idle);
   bool handle(const message::CreatePort &createPort);
   bool handle(const message::AddParameter &addParam);
   bool handle(const message::SetParameter &setParam);
   bool handle(const message::SetParameterChoices &setChoices);
   bool handle(const message::Kill &kill);
   bool handle(const message::AddObject &addObj);
   bool handle(const message::ObjectReceived &objRecv);
   bool handle(const message::Barrier &barrier);
   bool handle(const message::BarrierReached &barrierReached);
   bool handle(const message::ResetModuleIds &reset);

   std::string m_bindir;

   const int m_rank;
   const int m_size;
   const std::vector<std::string> m_hosts;

   struct Module {
      message::MessageQueue *sendQueue = NULL;
      message::MessageQueue *recvQueue = NULL;

      ~Module() {
         delete sendQueue;
         delete recvQueue;
      }
      bool local = false;
      int baseRank = 0;
   };
   typedef std::map<int, Module> RunningMap;
   RunningMap runningMap;

   int m_moduleCounter; //< used for module ids
   int m_executionCounter; //< incremented each time the pipeline is executed

   // barrier related stuff
   boost::mutex m_barrierMutex;
   boost::condition_variable m_barrierCondition;
   bool checkBarrier(int id) const;
   void barrierReached(int id);
   int m_barrierCounter;
   int m_activeBarrier;
   message::Message::uuid_t m_barrierUuid;
   int m_reachedBarriers;
   typedef std::set<int> ModuleSet;
   ModuleSet reachedSet;
};

} // namespace vistle

#endif
