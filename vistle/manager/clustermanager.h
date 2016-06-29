#ifndef CLUSTERMANAGER_H
#define CLUSTERMANAGER_H

#include <vector>
#include <map>

#include <util/directory.h>

#include <core/statetracker.h>
#include <core/message.h>
#include <core/messagequeue.h>

#include "portmanager.h"

namespace vistle {

namespace message {
   class Message;
   class MessageQueue;
}

class Parameter;

class ClusterManager {
   friend class Communicator;
   friend class DataManager;

 public:
   ClusterManager(int rank, const std::vector<std::string> &hosts);
   ~ClusterManager();

   bool scanModules(const std::string &dir);

   bool dispatch(bool &received);
   const StateTracker &state() const;

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
   boost::shared_ptr<Parameter> getParameter(int id, const std::string &name) const;

   PortManager &portManager() const;

   bool handle(const message::Message &msg);
   //bool handleData(const message::Message &msg);

 private:
   void queueMessage(const message::Message &msg);
   void replayMessages();
   bool isLocal(int id) const;
   std::vector<message::Buffer> m_messageQueue;

   boost::shared_ptr<PortManager> m_portManager;
   StateTracker m_stateTracker;
   message::Message::Type m_traceMessages;

   bool m_quitFlag;

   bool handlePriv(const message::Trace &trace);
   bool handlePriv(const message::Spawn &spawn);
   bool handlePriv(const message::Connect &connect);
   bool handlePriv(const message::Disconnect &disc);
   bool handlePriv(const message::ModuleExit &moduleExit);
   bool handlePriv(const message::Execute &exec);
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

   bool checkExecuteObject(int modId);

   const int m_rank;
   const int m_size;

   struct Module {
      message::MessageQueue *sendQueue;
      message::MessageQueue *recvQueue;
      int ranksStarted, ranksFinished;
      bool reducing;
      bool prepared, reduced;
      int busyCount;
      mutable bool blocked;
      mutable std::deque<message::Buffer> blockedMessages, blockers;
      std::deque<message::Buffer> delayedMessages;
      std::vector<int> objectCount; // no. of available object tuples on each rank

      Module(): sendQueue(nullptr), recvQueue(nullptr),
         ranksStarted(0), ranksFinished(0), reducing(false),
         prepared(false), reduced(true),
         busyCount(0), blocked(false)
         {}
      ~Module() {
         delete sendQueue;
         delete recvQueue;
      }
      void block(const message::Message &msg);
      void unblock(const message::Message &msg);
      bool send(const message::Message &msg) const;
      bool update() const;
      void delay(const message::Message &msg);
      bool processDelayed();
      bool haveDelayed() const;
   };
   typedef std::map<int, Module> RunningMap;
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
};

} // namespace vistle

#endif
