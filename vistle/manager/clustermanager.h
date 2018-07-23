#ifndef CLUSTERMANAGER_H
#define CLUSTERMANAGER_H

#include <vector>
#include <unordered_map>
#include <queue>

#include <util/directory.h>

#include <core/statetracker.h>
#include <core/message.h>
#include <core/messagequeue.h>
#include <core/parametermanager.h>
#include <core/parameter.h>

#include "portmanager.h"

#ifdef MODULE_THREAD
#include <module/module.h>
#include <boost/function.hpp>
#include <thread>
#endif

namespace vistle {

namespace message {
   class Message;
   class MessageQueue;
}

class Parameter;

class ClusterManager: public ParameterManager
{
   friend class Communicator;
   friend class DataManager;

 public:
   ClusterManager(int rank, const std::vector<std::string> &hosts);
   virtual ~ClusterManager();

   void init(); // to be called when message sending is possible

   bool dispatch(bool &received);
   const StateTracker &state() const;

   void sendParameterMessage(const message::Message &message) const override;
   bool sendMessage(int receiver, const message::Message &message, int destRank=-1) const;
   bool sendAll(const message::Message &message) const;
   bool sendAllLocal(const message::Message &message) const;
   bool sendAllOthers(int excluded, const message::Message &message, bool localOnly=false) const;
   bool sendUi(const message::Message &message) const;
   bool sendHub(const message::Message &message, int destHub=message::Id::Broadcast) const;

   bool quit();
   bool quitOk() const;

   int getRank() const;
   int getSize() const;

   std::vector<AvailableModule> availableModules() const;

   std::string getModuleName(int id) const;
   std::vector<std::string> getParameters(int id) const;
   std::shared_ptr<Parameter> getParameter(int id, const std::string &name) const;

   PortManager &portManager() const;

   bool handle(const message::Buffer &msg);
   //bool handleData(const message::Message &msg);

 private:
   void queueMessage(const message::Message &msg);
   void replayMessages();
   bool isLocal(int id) const;
   std::vector<message::Buffer> m_messageQueue;

   std::shared_ptr<PortManager> m_portManager;
   StateTracker m_stateTracker;
   message::Type m_traceMessages;

   struct CompModuleHeight {
      bool operator()(const StateTracker::Module &a, const StateTracker::Module &b) {
         return a.height > b.height;
      }
   };
   std::vector<StateTracker::Module> m_modulePriority;
   int m_modulePriorityChange = -1;

   bool m_quitFlag;

   bool handlePriv(const message::Trace &trace);
   bool handlePriv(const message::Spawn &spawn);
   bool handlePriv(const message::Connect &connect);
   bool handlePriv(const message::Disconnect &disc);
   bool handlePriv(const message::ModuleExit &moduleExit);
   bool handlePriv(const message::Execute &exec);
   bool handlePriv(const message::CancelExecute &cancel);
   bool handlePriv(const message::ExecutionProgress &prog);
   bool handlePriv(const message::Busy &busy);
   bool handlePriv(const message::Idle &idle);
   bool handlePriv(const message::SetParameter &setParam);
   bool handlePriv(const message::SetParameterChoices &setChoices);
   bool handlePriv(const message::AddObject &addObj, bool synthesized=false);
   bool handlePriv(const message::AddObjectCompleted &complete);
   bool handlePriv(const message::ObjectReceived &objRecv);
   bool handlePriv(const message::Barrier &barrier);
   bool handlePriv(const message::BarrierReached &barrierReached);
   bool handlePriv(const message::SendText &text);
   bool handlePriv(const message::RequestTunnel &tunnel);
   bool handlePriv(const message::Ping &ping);

   //! check whether objects are available for a module and compute() should be called
   bool checkExecuteObject(int modId);

   const int m_rank;
   const int m_size;

   struct Module {
#ifdef MODULE_THREAD
      typedef std::shared_ptr<vistle::Module> (NewModuleFunc)(const std::string &name, int id, mpi::communicator comm);
      boost::function<NewModuleFunc> newModule;
      std::shared_ptr<vistle::Module> instance;
      std::thread thread;
#endif
      std::shared_ptr<message::MessageQueue> sendQueue, recvQueue;
      int ranksStarted, ranksFinished;
      bool reducing;
      bool prepared, reduced;
      int busyCount;
      mutable bool blocked;
      mutable std::deque<message::Buffer> blockedMessages, blockers;
      std::deque<message::Buffer> delayedMessages;
      std::vector<int> objectCount; // no. of available object tuples on each rank

      Module(): ranksStarted(0), ranksFinished(0), reducing(false),
         prepared(false), reduced(true),
         busyCount(0), blocked(false)
         {}
      ~Module();

      void block(const message::Message &msg);
      void unblock(const message::Message &msg);
      bool send(const message::Message &msg) const;
      bool update() const;
      void delay(const message::Message &msg);
      bool processDelayed();
      bool haveDelayed() const;
   };
   typedef std::unordered_map<int, Module> RunningMap;
   RunningMap runningMap;
   int numRunning() const;
   bool isReadyForExecute(int modId) const;

   // barrier related stuff
   bool checkBarrier(const message::uuid_t &uuid) const;
   void barrierReached(const message::uuid_t &uuid);
   bool m_barrierActive;
   message::uuid_t m_barrierUuid;
   int m_reachedBarriers;
   typedef std::set<int> ModuleSet;
   ModuleSet reachedSet;

   IntParameter *m_compressionMode = nullptr;
   FloatParameter *m_zfpRate = nullptr;
   IntParameter *m_zfpPrecision = nullptr;
   FloatParameter *m_zfpAccuracy = nullptr;
};

} // namespace vistle

#endif
