#ifndef MODULEMANAGER_H
#define MODULEMANAGER_H

#include <vector>
#include <map>

#include <boost/interprocess/ipc/message_queue.hpp>

#include <util/directory.h>

#include <core/statetracker.h>
#include <core/message.h>
#include <core/messagequeue.h>

#include "portmanager.h"
#include "export.h"

namespace vistle {

namespace message {
   class Message;
   class MessageQueue;
}

class Parameter;

class V_CONTROLEXPORT ModuleManager {
   friend class Communicator;

 public:
   ModuleManager(int argc, char *argv[], int rank, const std::vector<std::string> &hosts);
   ~ModuleManager();
   static ModuleManager &the();

   bool scanModules(const std::string &dir);

   bool dispatch(bool &received);

   bool sendMessage(int receiver, const message::Message &message) const;
   bool sendAll(const message::Message &message) const;
   bool sendAllOthers(int excluded, const message::Message &message) const;
   bool sendUi(const message::Message &message) const;
   bool sendHub(const message::Message &message) const;

   bool quit();
   bool quitOk() const;

   int getRank() const;
   int getSize() const;

   void resetModuleCounter();
   int newModuleID();
   int currentExecutionCount();
   int newExecutionCount();

   std::vector<AvailableModule> availableModules() const;

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
   bool m_quitFlag;

   // only used by Communicator
   bool handle(const message::Ping &ping);
   bool handle(const message::Pong &pong);
   bool handle(const message::Trace &trace);
   bool handle(const message::Spawn &spawn);
   bool handle(const message::Started &started);
   bool handle(const message::Connect &connect);
   bool handle(const message::Disconnect &disc);
   bool handle(const message::ModuleExit &moduleExit);
   bool handle(const message::Compute &compute);
   bool handle(const message::Reduce &reduce);
   bool handle(const message::ExecutionProgress &prog);
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
   bool handle(const message::ObjectReceivePolicy &receivePolicy);
   bool handle(const message::SchedulingPolicy &schedulingPolicy);
   bool handle(const message::ReducePolicy &reducePolicy);
   bool handle(const message::ModuleAvailable &avail);

   std::string m_bindir;

   const int m_rank;
   const int m_size;
   const std::vector<std::string> m_hosts;

   AvailableMap m_availableMap;

   struct Module {
      message::MessageQueue *sendQueue;
      message::MessageQueue *recvQueue;

      Module(): sendQueue(nullptr), recvQueue(nullptr),
         local(false), baseRank(0),
         ranksStarted(0), ranksFinished(0), reducing(false),
         objectPolicy(message::ObjectReceivePolicy::Single), schedulingPolicy(message::SchedulingPolicy::Single), reducePolicy(message::ReducePolicy::Never)
         {}
      ~Module() {
         delete sendQueue;
         delete recvQueue;
      }
      bool local;
      int baseRank;
      int ranksStarted, ranksFinished;
      bool reducing;
      message::ObjectReceivePolicy::Policy objectPolicy;
      message::SchedulingPolicy::Schedule schedulingPolicy;
      message::ReducePolicy::Reduce reducePolicy;
   };
   typedef std::map<int, Module> RunningMap;
   RunningMap runningMap;
   int numRunning() const;

   int m_moduleCounter; //< used for module ids
   int m_executionCounter; //< incremented each time the pipeline is executed

   // barrier related stuff
   bool checkBarrier(const message::uuid_t &uuid) const;
   void barrierReached(const message::uuid_t &uuid);
   bool m_barrierActive;
   message::uuid_t m_barrierUuid;
   int m_reachedBarriers;
   typedef std::set<int> ModuleSet;
   ModuleSet reachedSet;
};

} // namespace vistle

#endif
