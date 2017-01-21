/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <util/sysdep.h>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <mpi.h>

#include <sys/types.h>

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <queue>
#include <future>
#include <numeric>

#include <core/message.h>
#include <core/messagequeue.h>
#include <core/messagerouter.h>
#include <core/object.h>
#include <core/shm.h>
#include <core/parameter.h>
#include <util/directory.h>

#include "clustermanager.h"
#include "datamanager.h"
#include "communicator.h"

//#define QUEUE_DEBUG
#define BARRIER_DEBUG
//#define DEBUG

#define CERR \
   std::cerr << "ClusterManager [" << m_rank << "/" << m_size << "] "

namespace bi = boost::interprocess;

namespace vistle {

using message::Id;

void ClusterManager::Module::block(const message::Message &msg) {
   std::cerr << "BLOCK: " << msg << std::endl;
   blocked = true;
   blockers.push_back(message::Buffer(msg));
}

void ClusterManager::Module::unblock(const message::Message &msg) {
   //std::cerr << "UNBLOCK: " << msg << " (blocked: " << blocked << ")" << std::endl;
   vassert(blocked);
   vassert(!blockers.empty());

   auto pred = [msg](const message::Buffer &buf) -> bool {
      return buf.uuid() == msg.uuid()
            && buf.type() == msg.type();
   };

   if (blocked) {
      if (pred(blockers.front())) {
         //std::cerr << "UNBLOCK: found as frontmost blocker" << std::endl;
         blockers.pop_front();
         vassert(blockedMessages.front().uuid() == msg.uuid()
                && blockedMessages.front().type() == msg.type());
         blockedMessages.pop_front();
         sendQueue->send(msg);
         if (blockers.empty()) {
            //std::cerr << "UNBLOCK: completely unblocked" << std::endl;
            blocked = false;
            while (!blockedMessages.empty()) {
               sendQueue->send(blockedMessages.front());
               blockedMessages.pop_front();
            }
         } else {
            const auto &uuid = blockers.front().uuid();
            while (blockedMessages.front().uuid() != uuid) {
               sendQueue->send(blockedMessages.front());
               blockedMessages.pop_front();
            }
         }
      } else {
         //std::cerr << "UNBLOCK: frontmost blocker: " << blockers.front() << std::endl;
         auto it = std::find_if(blockers.begin(), blockers.end(), pred);
         vassert(it != blockers.end());
         if (it != blockers.end()) {
            //std::cerr << "UNBLOCK: found in blockers" << std::endl;
            blockers.erase(it);
         }
         it = std::find_if(blockedMessages.begin(), blockedMessages.end(), pred);
         vassert (it != blockedMessages.end());
         if (it != blockedMessages.end()) {
            //std::cerr << "UNBLOCK: updating message" << std::endl;
            *it = message::Buffer(msg);
         }
      }
   }
}

bool ClusterManager::Module::send(const message::Message &msg) const {
   if (blocked) {
      blockedMessages.emplace_back(msg);
      return true;
   } else if (sendQueue) {
      return sendQueue->send(msg);
   }
   return false;
}

bool ClusterManager::Module::update() const {
    if (sendQueue)
        return sendQueue->progress();
    return false;
}

void ClusterManager::Module::delay(const message::Message &msg) {
    delayedMessages.push_back(msg);
}

bool  ClusterManager::Module::processDelayed() {

    while (haveDelayed()) {
        bool ret = true;
        if (Communicator::the().getRank() == 0) {
            if (ranksStarted == 0)
            {
                auto &msg = delayedMessages.front();
                auto type = msg.type();
                ret = Communicator::the().broadcastAndHandleMessage(delayedMessages.front());
                delayedMessages.pop_front();
                if (type == message::EXECUTE)
                    break;
            }
        }
        if (!ret)
            return false;
    }
    return true;
}

bool ClusterManager::Module::haveDelayed() const {
    return !delayedMessages.empty();
}

ClusterManager::ClusterManager(int r, const std::vector<std::string> &hosts)
: m_portManager(new PortManager(this))
, m_stateTracker(std::string("ClusterManager state rk")+boost::lexical_cast<std::string>(r), m_portManager)
, m_traceMessages(message::INVALID)
, m_quitFlag(false)
, m_rank(r)
, m_size(hosts.size())
, m_barrierActive(false)
{
    m_portManager->setTracker(&m_stateTracker);
}

ClusterManager::~ClusterManager() {

    m_portManager->setTracker(nullptr);
}

const StateTracker &ClusterManager::state() const {
   return m_stateTracker;
}

int ClusterManager::getRank() const {

   return m_rank;
}

int ClusterManager::getSize() const {

   return m_size;
}

bool ClusterManager::isLocal(int id) const {

   if (id == Id::LocalHub)
      return true;
   if (Id::isHub(id)) {
      return (id == Communicator::the().hubId());
   }
   int hub = m_stateTracker.getHub(id);
   return hub == Communicator::the().hubId();
}

std::vector<AvailableModule> ClusterManager::availableModules() const {

   return m_stateTracker.availableModules();
}

bool ClusterManager::checkBarrier(const message::uuid_t &uuid) const {

   vassert(m_barrierActive);
   int numLocal = 0;
   for (const auto &m: m_stateTracker.runningMap) {
      if (m.second.hub == Communicator::the().hubId())
         ++numLocal;
   }
#ifdef BARRIER_DEBUG
   CERR << "checkBarrier " << uuid << ": #local=" << numLocal << ", #reached=" << reachedSet.size() << std::endl;
#endif
   if (reachedSet.size() == numLocal)
      return true;

   return false;
}

void ClusterManager::barrierReached(const message::uuid_t &uuid) {

   vassert(m_barrierActive);
   MPI_Barrier(MPI_COMM_WORLD);
   reachedSet.clear();
   CERR << "Barrier [" << uuid << "] reached" << std::endl;
   message::BarrierReached m(uuid);
   m.setDestId(message::Id::MasterHub);
   if (getRank() == 0)
      sendHub(m, Id::MasterHub);
}

std::string ClusterManager::getModuleName(int id) const {

   return m_stateTracker.getModuleName(id);
}


bool ClusterManager::dispatch(bool &received) {

   bool done = false;

   if (m_modulePriorityChange != m_stateTracker.graphChangeCount()) {

       // handle messages from modules closer to sink first
       // - should allow for objects to travel through the pipeline more quickly
       while (!m_modulePriority.empty()) {
           m_modulePriority.pop();
       }
       m_modulePriorityChange = m_stateTracker.graphChangeCount();
       for (auto m: m_stateTracker.runningMap) {
           const auto &mod = m.second;
           m_modulePriority.emplace(mod);
       }
   }

   auto modules = m_modulePriority;

   // handle messages from modules
   while (!modules.empty()) {

      auto mod = modules.top();
      modules.pop();
      const int modId = mod.id;

      // keep messages from modules that have already reached a barrier on hold
      if (reachedSet.find(modId) != reachedSet.end())
         continue;

      if (mod.hub == Communicator::the().hubId()) {
         bool recv = false;
         message::Buffer buf;
         message::MessageQueue *mq = nullptr;
         auto it = runningMap.find(modId);
         if (it != runningMap.end()) {
            it->second.update();
            mq = it->second.recvQueue;
         }
         if (mq) {
            try {
               recv = mq->tryReceive(buf);
            } catch (boost::interprocess::interprocess_exception &ex) {
               CERR << "receive mq " << ex.what() << std::endl;
               exit(-1);
            }
         }

         if (recv) {
            received = true;
            if (!Communicator::the().handleMessage(buf))
               done = true;
         }
      }
   }

   if (m_quitFlag) {
      if (numRunning() == 0)
         return false;

#if 0
      CERR << numRunning() << " modules still running..." << std::endl;
#endif
   }

   return !done;
}

bool ClusterManager::sendAll(const message::Message &message) const {

   // no module has id Invalid
   return sendAllOthers(message::Id::Invalid, message);
}

bool ClusterManager::sendAllLocal(const message::Message &message) const {

   // no module has id Invalid
   return sendAllOthers(message::Id::Invalid, message, true);
}

bool ClusterManager::sendAllOthers(int excluded, const message::Message &message, bool localOnly) const {

   message::Buffer buf(message);
   if (!localOnly) {
      buf.setDestId(Id::ForBroadcast);
      if (Communicator::the().isMaster()) {
         if (getRank() == 0)
            sendHub(buf);
      } else {
         int senderHub = message.senderId();
         if (senderHub >= Id::ModuleBase)
            senderHub = m_stateTracker.getHub(senderHub);
         if (senderHub == Communicator::the().hubId()) {
            if (getRank() == 0)
               sendHub(buf);
         }
      }
      return true;
   }

   // handle messages to modules
   for(auto it = runningMap.begin(), next = it;
         it != runningMap.end();
         it = next) {
      // modules might be removed during message processing
      next = it;
      ++next;

      const int modId = it->first;
      if (modId == excluded)
         continue;
      const auto &mod = it->second;
      const int hub = m_stateTracker.getHub(modId);

      if (hub == Communicator::the().hubId()) {
         mod.send(message);
      }
   }

   return true;
}

bool ClusterManager::sendUi(const message::Message &message) const {

   return sendHub(message);
}

bool ClusterManager::sendHub(const message::Message &message, int destHub) const {

    if (getRank()!=0 && !message::Router::the().toRank0(message)) {
        return true;
    }

    message::Buffer buf(message);
    if (Id::isHub(destHub))
       buf.setDestId(destHub);
    return Communicator::the().sendHub(buf);
}

bool ClusterManager::sendMessage(const int moduleId, const message::Message &message, int destRank) const {

   const int hub = m_stateTracker.getHub(moduleId);

   if (hub == Communicator::the().hubId()) {
      //std::cerr << "local send to " << moduleId << ": " << message << std::endl;
      RunningMap::const_iterator it = runningMap.find(moduleId);
      if (it == runningMap.end()) {
         CERR << "sendMessage: module " << moduleId << " not found" << std::endl;
         std::cerr << "  message: " << message << std::endl;
         return true;
      }

      if (destRank == -1 || destRank == getRank()) {
         auto &mod = it->second;
         mod.send(message);
      } else {
         Communicator::the().sendMessage(moduleId, message, destRank);
      }
   } else {
      std::cerr << "remote send to " << moduleId << ": " << message << std::endl;
      message::Buffer buf(message);
      buf.setDestId(moduleId);
      buf.setDestRank(destRank);
      sendHub(buf);
   }

   return true;
}

bool ClusterManager::handle(const message::Buffer &message) {

   using namespace vistle::message;

   if (message.destId() == Id::ForBroadcast) {
       return sendHub(message);
   }

   if (message.type() == m_traceMessages || m_traceMessages==ANY) {
       CERR << "handle: " << message << std::endl;
   }

   switch (message.type()) {
      case CONNECT:
      case DISCONNECT:
         // handled in handlePriv(...)
         break;
      default:
         m_stateTracker.handle(message);
         break;
   }

   bool result = true;
   const int hubId = Communicator::the().hubId();

   int senderHub = message.senderId();
   if (senderHub >= Id::ModuleBase)
      senderHub = m_stateTracker.getHub(senderHub);
   int destHub = message.destId();
   if (destHub >= Id::ModuleBase)
      destHub = m_stateTracker.getHub(destHub);
   if (message.typeFlags() & Broadcast || message.destId() == Id::Broadcast) {
#if 0
      if (message.senderId() != hubId && senderHub == hubId) {
         CERR << "BC: " << message << std::endl;
         if (getRank() == 0)
            sendHub(message);
      }
#endif
      if (message.typeFlags() & BroadcastModule) {
         sendAllLocal(message);
      }
   }
   if (message::Id::isModule(message.destId())) {
      if (destHub == hubId) {
         //CERR << "module: " << message << std::endl;
         if (message.type() != message::EXECUTE) {
            return sendMessage(message.destId(), message);
         }
      } else {
         return sendHub(message);
      }
   }
   if (message::Id::isHub(message.destId())) {
       if (destHub != hubId || message.type() == message::EXECUTE) {
           return sendHub(message);
       }
   }

   switch (message.type()) {

      case IDENTIFY: {

         const Identify &id = message.as<Identify>();
         CERR << "Identify message: " << id << std::endl;
         vassert(id.identity() == Identify::REQUEST);
         if (getRank() == 0) {
            sendHub(Identify(Identify::MANAGER));
            auto avail = availableModules();
            for(const auto &mod: avail) {
               sendHub(message::ModuleAvailable(hubId, mod.name, mod.path));
            }
         }
         break;
      }

      case message::QUIT: {

         result = false;
         break;
      }

      case message::TRACE: {
         const Trace &trace = message.as<Trace>();
         result = handlePriv(trace);
         break;
      }

      case message::SPAWN: {

         const message::Spawn &spawn = message.as<Spawn>();
         result = handlePriv(spawn);
         break;
      }

      case message::CONNECT: {

         const message::Connect &connect = message.as<Connect>();
         result = handlePriv(connect);
         break;
      }

      case message::DISCONNECT: {

         const message::Disconnect &disc = message.as<Disconnect>();
         result = handlePriv(disc);
         break;
      }

      case message::MODULEEXIT: {

         const message::ModuleExit &moduleExit = message.as<ModuleExit>();
         result = handlePriv(moduleExit);
         break;
      }

      case message::EXECUTE: {

         const message::Execute &exec = message.as<Execute>();
         result = handlePriv(exec);
         break;
      }

      case message::ADDOBJECT: {

         const message::AddObject &m = message.as<AddObject>();
         result = handlePriv(m);
         break;
      }

      case message::ADDOBJECTCOMPLETED: {
         const message::AddObjectCompleted &m = message.as<AddObjectCompleted>();
         result = handlePriv(m);
         break;
      }

      case message::EXECUTIONPROGRESS: {

         const message::ExecutionProgress &prog = message.as<ExecutionProgress>();
         result = handlePriv(prog);
         break;
      }

      case message::BUSY: {

         const message::Busy &busy = message.as<Busy>();
         result = handlePriv(busy);
         break;
      }

      case message::IDLE: {

         const message::Idle &idle = message.as<Idle>();
         result = handlePriv(idle);
         break;
      }

      case message::OBJECTRECEIVED: {
         const message::ObjectReceived &m = message.as<ObjectReceived>();
         result = handlePriv(m);
         break;
      }

      case message::SETPARAMETER: {

         const message::SetParameter &m = message.as<SetParameter>();
         result = handlePriv(m);
         break;
      }

      case message::SETPARAMETERCHOICES: {
         const message::SetParameterChoices &m = message.as<SetParameterChoices>();
         result = handlePriv(m);
         break;
      }

      case message::BARRIER: {

         const message::Barrier &m = message.as<Barrier>();
         result = handlePriv(m);
         break;
      }

      case message::BARRIERREACHED: {

         const message::BarrierReached &m = message.as<BarrierReached>();
         result = handlePriv(m);
         break;
      }

      case message::SENDTEXT: {
         const message::SendText &m = message.as<SendText>();
         result = handlePriv(m);
         break;
      }

      case message::REQUESTTUNNEL: {
         const message::RequestTunnel &m = message.as<RequestTunnel>();
         result = handlePriv(m);
         break;
      }

      case message::PING: {
         const message::Ping &m = message.as<Ping>();
         result = handlePriv(m);
         break;
      }

      case message::ADDHUB:
      case message::REMOVESLAVE:
      case message::STARTED:
      case message::ADDPORT:
      case message::ADDPARAMETER:
      case message::REMOVEPARAMETER:
      case message::MODULEAVAILABLE:
      case message::REPLAYFINISHED:
      case message::REDUCEPOLICY:
      case message::SCHEDULINGPOLICY:
      case message::OBJECTRECEIVEPOLICY:
      case message::PONG:
         break;

      default:

         CERR << "unhandled message from (id "
            << message.senderId() << " rank " << message.rank() << ") "
            << "type " << message.type()
            << std::endl;

         break;

   }

   if (result) {
      if (message.typeFlags() & TriggerQueue) {
         replayMessages();
      }
   } else {
      if (message.typeFlags() & QueueIfUnhandled) {
         queueMessage(message);
         result = true;
      }
   }

   return result;
}

bool ClusterManager::handlePriv(const message::Trace &trace) {

   CERR << "handle: " << trace << std::endl;

   if (trace.module() >= Id::ModuleBase) {
      sendMessage(trace.module(), trace);
   } else if (trace.module() == Id::Broadcast) {
      sendAllLocal(trace);
   }

   if (trace.module() == Id::Broadcast || trace.module() == Communicator::the().hubId()) {
      if (trace.on())
         m_traceMessages = trace.messageType();
      else
         m_traceMessages = message::INVALID;
   }

   return true;
}

bool ClusterManager::handlePriv(const message::Spawn &spawn) {

   if (spawn.spawnId() == Id::Invalid) {
      // ignore messages where master hub did not yet create an id
      return true;
   }
   sendAllLocal(spawn);
   if (spawn.destId() != Communicator::the().hubId()) {
      return true;
   }

   int newId = spawn.spawnId();

   Module &mod = runningMap[newId];
   std::string smqName = message::MessageQueue::createName("smq", newId, m_rank);
   std::string rmqName = message::MessageQueue::createName("rmq", newId, m_rank);

   try {
      mod.sendQueue = message::MessageQueue::create(smqName);
      mod.recvQueue = message::MessageQueue::create(rmqName);
   } catch (bi::interprocess_exception &ex) {

      delete mod.sendQueue;
      mod.sendQueue = nullptr;
      delete mod.recvQueue;
      mod.recvQueue = nullptr;
      CERR << "spawn mq " << ex.what() << std::endl;
      exit(-1);
   }

   mod.sendQueue->makeNonBlocking();

   MPI_Barrier(MPI_COMM_WORLD);

   message::SpawnPrepared prep(spawn);
   prep.setDestId(Id::LocalHub);
   if (getRank() == 0)
      sendHub(prep);

   // inform newly started module about current parameter values of other modules
   auto state = m_stateTracker.getState();
   for (const auto &m: state) {
      sendMessage(newId, m);
   }

   return true;
}

bool ClusterManager::handlePriv(const message::Connect &connect) {

   int modFrom = connect.getModuleA();
   int modTo = connect.getModuleB();
   const char *portFrom = connect.getPortAName();
   const char *portTo = connect.getPortBName();
   const Port *from = portManager().getPort(modFrom, portFrom);
   const Port *to = portManager().getPort(modTo, portTo);

   message::Connect c = connect;
   if (from && to && from->getType() == Port::INPUT && to->getType() == Port::OUTPUT) {
      c.reverse();
      std::swap(modFrom, modTo);
      std::swap(portFrom, portTo);
   }

   if (m_stateTracker.handle(c)) {
      // inform modules about connections
      if (Communicator::the().isMaster()) {
         sendMessage(modFrom, c);
         sendMessage(modTo, c);
         sendUi(c);
      }
   } else {
      return false;
   }
   return true;
}

bool ClusterManager::handlePriv(const message::Disconnect &disconnect) {

   int modFrom = disconnect.getModuleA();
   int modTo = disconnect.getModuleB();
   const char *portFrom = disconnect.getPortAName();
   const char *portTo = disconnect.getPortBName();
   const Port *from = portManager().findPort(modFrom, portFrom);
   const Port *to = portManager().findPort(modTo, portTo);

   if (!from) {
      CERR << " Did not find source port: " << disconnect << std::endl;
      return true;
   }
   if (!to) {
      CERR << " Did not find destination port: " << disconnect << std::endl;
      return true;
   }

   message::Disconnect d = disconnect;
   if (from->getType() == Port::INPUT && to->getType() == Port::OUTPUT) {
      d.reverse();
      std::swap(modFrom, modTo);
      std::swap(portFrom, portTo);
   }
   
   if (m_stateTracker.handle(d)) {

       if (Communicator::the().isMaster()) {
         sendMessage(modFrom, d);
         sendMessage(modTo, d);
         sendUi(d);
      }
   } else {

      if (!m_messageQueue.empty()) {
         // only if messages are already queued, there is a chance that this
         // connection might still be established
         return false;
      }
   }

   return true;
}

bool ClusterManager::handlePriv(const message::ModuleExit &moduleExit) {

   const int mod = moduleExit.senderId();
   sendAllOthers(mod, moduleExit, true);
   const bool local = isLocal(mod);

   if (!moduleExit.isForwarded()) {

      if (local) {
          if (runningMap.find(mod) == runningMap.end()) {
              CERR << " Module [" << mod << "] quit, but not found in running map" << std::endl;
              return true;
          }

          ModuleSet::iterator it = reachedSet.find(mod);
          if (it != reachedSet.end()) {
              reachedSet.erase(it);
          } else {
              if (m_barrierActive && checkBarrier(m_barrierUuid))
                  barrierReached(m_barrierUuid);
          }
      }

      if (m_rank == 0) {
         message::ModuleExit exit = moduleExit;
         exit.setForwarded();
         sendHub(exit);
         if (!Communicator::the().broadcastAndHandleMessage(exit))
            return false;
      }
   }

   //CERR << " Module [" << mod << "] quit" << std::endl;

   { 
      RunningMap::iterator it = runningMap.find(mod);
      if (it != runningMap.end()) {
         runningMap.erase(it);
      } else {
         //CERR << " Module [" << mod << "] not found in map" << std::endl;
      }
   }

   return true;
}

bool ClusterManager::handlePriv(const message::Execute &exec) {

   vassert (exec.getModule() >= Id::ModuleBase);
   RunningMap::iterator i = runningMap.find(exec.getModule());
   if (i != runningMap.end()) {
      auto &mod = i->second;
      switch(exec.what()) {
      case message::Execute::Prepare: {
         vassert(!mod.prepared);
         vassert(mod.reduced);
         mod.prepared = true;
         mod.reduced = false;
         mod.send(exec);
         CERR << "sent prepare to " << exec.getModule() << ", checking for execution" << std::endl;
         checkExecuteObject(exec.getModule());
         break;
      }
      case message::Execute::Reduce: {
         vassert(mod.prepared);
         vassert(!mod.reduced);
         mod.prepared = false;
         mod.reduced = true;
         mod.send(exec);
         break;
      }
      case message::Execute::ComputeExecute: {
         if (exec.isBroadcast()) {
             mod.send(exec);
             vassert(!mod.prepared);
             mod.prepared = false;
             mod.reduced = true;
         } else if (Communicator::the().getRank() == 0) {
             CERR << "non-broadcast Execute: " << exec << std::endl;
             if (mod.ranksStarted > 0) {
                 mod.delay(exec);
             } else {
                 vassert(!mod.prepared);
                 mod.prepared = false;
                 mod.reduced = true;
                 message::Buffer buf(exec);
                 buf.setBroadcast(true);
                 Communicator::the().broadcastAndHandleMessage(buf);
             }
         }
         break;
      }
      case message::Execute::ComputeObject: {
         //CERR << exec << std::endl;
         vassert(mod.prepared);
         vassert(!mod.reduced);
         auto it = m_stateTracker.runningMap.find(exec.getModule());
         auto pol = message::SchedulingPolicy::Single;
         if (it != m_stateTracker.runningMap.end()) {
             pol = it->second.schedulingPolicy;
         }
         if (exec.isBroadcast() || pol == message::SchedulingPolicy::Single) {
             mod.reduced = false;
             mod.send(exec);
         } else {
             if (m_rank == 0) {
                 bool doExec = pol == message::SchedulingPolicy::Gang;
                 if (pol == message::SchedulingPolicy::LazyGang) {
                     if (mod.objectCount.size() < getSize())
                         mod.objectCount.resize(getSize());
                     ++mod.objectCount[exec.rank()];
                     int numObjects = std::accumulate(mod.objectCount.begin(), mod.objectCount.end(), 0);
                     if (numObjects>0 && numObjects>=getSize()*.2) {
                         doExec = true;
                         for (auto &c: mod.objectCount) {
                             if (c > 0) {
                                 --c;
                             }
                         }
                     }
                 }
                 if (doExec) {
                     //CERR << "having " << numObjects << ", executing " << exec.getModule() << std::endl;
                     message::Buffer buf(exec);
                     buf.setBroadcast(true);
                     Communicator::the().broadcastAndHandleMessage(buf);
                 }
             }
         }
         break;
      }
      }
   }

   return true;
}

bool ClusterManager::handlePriv(const message::AddObject &addObj, bool synthesized) {


   const bool localAdd = isLocal(addObj.senderId());
   //CERR << "ADDOBJECT: " << addObj << ", local=" << localAdd << ", synthesized=" << synthesized << std::endl;
   Object::const_ptr obj;

   bool onThisRank = false;
   int destRank = -1;
   if (localAdd) {
       if (addObj.rank() == getRank()) {
           onThisRank = true;
           obj = addObj.takeObject();
       }
       //CERR << "ADDOBJECT: local, name=" << obj->getName() << ", refcount=" << obj->refcount() << std::endl;
   } else {
       int block = addObj.meta().block();
       if (block >= 0) {
           destRank = block % getSize();
       }
       onThisRank = destRank==getRank() || (getRank() == 0 && destRank == -1);

      //CERR << "ADDOBJECT from remote, handling on rank " << destRank << std::endl;
      if (onThisRank) {
         obj = addObj.getObject();
         if (!obj) {
            vassert(!synthesized);
            //CERR << "AddObject: have to request " << addObj.objectName() << std::endl;
            Communicator::the().dataManager().requestObject(addObj, addObj.objectName(), []() -> void { /* FIXME */ });
         }
      } else {
          return true;
      }
   }

   vassert(!(synthesized && localAdd));
   if (synthesized || (localAdd && onThisRank)) {
      vassert(obj);
   }
   vassert(!obj || (obj->refcount() >= 1 && (localAdd || synthesized)));

   const Port *port = portManager().findPort(addObj.senderId(), addObj.getSenderPort());
   if (!port) {
      //CERR << "AddObject [" << addObj.objectName() << "] to port [" << addObj.getSenderPort() << "] of [" << addObj.senderId() << "]: port not found" << std::endl;
      vassert(port);
      return true;
   }
   const Port::ConstPortSet *list = portManager().getConnectionList(port);
   if (!list) {
      //CERR << "AddObject [" << addObj.objectName() << "] to port [" << addObj.getSenderPort() << "] of [" << addObj.senderId() << "]: connection list not found" << std::endl;
      vassert(list);
      return true;
   }

   std::set<int> receivingHubs; // make sure that message is only sent once per remote hub
   for (const Port *destPort: *list) {
      int destId = destPort->getModuleID();
      message::AddObject a(addObj);

      if (!isLocal(destId)) {
         if (localAdd) {
            // if object was generated locally, forward message to remote hubs with connected modules
            const int hub = m_stateTracker.getHub(destId);
            if (receivingHubs.find(hub) == receivingHubs.end()) {
               a.setDestId(hub);
               sendHub(a, hub);
               receivingHubs.insert(hub);
               Communicator::the().dataManager().prepareTransfer(a);
            }
         }
         continue;
      }

      if (!obj) {
         // block messages of receiving module until remote object is available
         vassert(!synthesized);
         vassert(!localAdd);
         auto it = runningMap.find(destId);
         if (it != runningMap.end()) {
            it->second.block(a);
         }
      } else if (synthesized) {
         // remote object available: unblock messages of receiving module and update AddObject message - all further messages have already been queued
         auto it = runningMap.find(destId);
         if (it != runningMap.end()) {
            message::AddObject add(a.getSenderPort(), obj, destPort->getName());
            add.setUuid(a.uuid());
            add.setDestId(destId);
            add.setSenderId(a.senderId());
            it->second.unblock(add);
         }
         continue;
      }

      a.setDestId(destId);
      a.setDestPort(destPort->getName());
      if (isLocal(destId) && localAdd && onThisRank) {
          a.ref();
      }
      sendMessage(destId, a);
      portManager().addObject(destPort);

      if (!checkExecuteObject(destId))
          return false;

      auto it = m_stateTracker.runningMap.find(destId);
      if (it == m_stateTracker.runningMap.end()) {
         CERR << "port connection to module " << destId << ":" << destPort->getName() << ", which is not running" << std::endl;
         vassert("port connection to module that is not running" == 0);
         continue;
      }
      auto &destMod = it->second;
      if (destMod.objectPolicy == message::ObjectReceivePolicy::NotifyAll
          || destMod.objectPolicy == message::ObjectReceivePolicy::Distribute) {
         message::ObjectReceived recv(addObj, addObj.getDestPort());
         recv.setUuid(addObj.uuid());
         recv.setSenderId(destId);

         if (!Communicator::the().broadcastAndHandleMessage(recv))
            return false;

      }
   }

   return true;
}

bool ClusterManager::checkExecuteObject(int destId) {

   if (!isReadyForExecute(destId))
       return true;

   for (const auto input: portManager().getConnectedInputPorts(destId)) {
       if (!portManager().hasObject(input)) {
           return true;
       }
   }
   for (const auto input: portManager().getConnectedInputPorts(destId))
       portManager().popObject(input);

   auto it = m_stateTracker.runningMap.find(destId);
   if (it == m_stateTracker.runningMap.end()) {
      CERR << "port connection to module that is not running" << std::endl;
      vassert("port connection to module that is not running" == 0);
      return true;
   }
   auto &destMod = it->second;
   message::Execute c(message::Execute::ComputeObject, destId);
   c.setDestId(destId);
   //c.setUuid(addObj.uuid());
   if (destMod.schedulingPolicy == message::SchedulingPolicy::Single) {
      sendMessage(destId, c);
   } else if (destMod.schedulingPolicy == message::SchedulingPolicy::Gang) {
      c.setAllRanks(true);
      if (!Communicator::the().broadcastAndHandleMessage(c))
         return false;
   } else if (destMod.schedulingPolicy == message::SchedulingPolicy::LazyGang) {
       if (getRank() == 0) {
           handle(c);
       } else {
           if (!Communicator::the().forwardToMaster(c))
               return false;
       }
   }

   return checkExecuteObject(destId);
}

bool ClusterManager::handlePriv(const message::AddObjectCompleted &complete) {

    return Communicator::the().dataManager().completeTransfer(complete);
}

bool ClusterManager::handlePriv(const message::ExecutionProgress &prog) {

   const bool localSender = m_stateTracker.getHub(prog.senderId()) == Communicator::the().hubId();
   RunningMap::iterator i = runningMap.find(prog.senderId());
   if (i == runningMap.end()) {
      vassert(localSender == false);
   }

   auto i2 = m_stateTracker.runningMap.find(prog.senderId());
   if (i2 == m_stateTracker.runningMap.end()) {
      CERR << "module " << prog.senderId() << " not found" << std::endl;
      return false;
   }
   auto &mod = i->second;
   auto &modState = i2->second;

   // initiate reduction if requested by module

   bool handleOnMaster = false;
   // forward message to remote hubs...
   std::set<int> receivingHubs; // ...but make sure that message is only sent once per remote hub
   if (localSender) {
      for (auto output: portManager().getConnectedOutputPorts(prog.senderId())) {
         const Port::ConstPortSet *list = portManager().getConnectionList(output);
         for (const Port *destPort: *list) {
            int destId = destPort->getModuleID();
            if (!isLocal(destId)) {
               int hub = m_stateTracker.getHub(destId);
               receivingHubs.insert(hub);
            }
            const auto it = m_stateTracker.runningMap.find(destId);
            if (it == m_stateTracker.runningMap.end()) {
                CERR << "did not find " << destId << " in runningMap, but it is connected to " << prog.senderId() << ":" << output->getName() << std::endl;
                abort();
            }
            vassert(it != m_stateTracker.runningMap.end());
            const auto &destState = it->second;
            if (destState.reducePolicy != message::ReducePolicy::Never && destState.reducePolicy != message::ReducePolicy::Locally) {
                handleOnMaster = true;
            }
         }
      }
   }
   if (!receivingHubs.empty())
       handleOnMaster = true;

   const bool localReduce = modState.reducePolicy == message::ReducePolicy::Locally || modState.reducePolicy == message::ReducePolicy::Never;
   if (!localReduce)
       handleOnMaster = true;
   if (localSender && handleOnMaster && m_rank != 0) {
      //CERR << "exec progr: forward to master" << std::endl;
      return Communicator::the().forwardToMaster(prog);
   }

   bool readyForPrepare = false, readyForReduce = false;
   bool unqueueExecute = false;
   //CERR << "ExecutionProgress " << prog.stage() << " received from " << prog.senderId() << ":" << prog.rank() << std::endl;
   switch (prog.stage()) {
      case message::ExecutionProgress::Start: {
         readyForPrepare = true;
         if (handleOnMaster && localSender) {
             vassert(mod.ranksFinished < m_size);
             ++mod.ranksStarted;
             readyForPrepare = mod.ranksStarted==m_size;
         }
         break;
      }

      case message::ExecutionProgress::Finish: {
         readyForReduce = true;
         if (handleOnMaster && localSender) {
             ++mod.ranksFinished;
             if (mod.ranksFinished == m_size) {
                if (mod.ranksStarted != m_size) {
                   CERR << "mismatch for module " << prog.senderId() << ": m_size=" << m_size << ", started=" << mod.ranksStarted << std::endl;
                }
                vassert(mod.ranksStarted >= m_size);
                mod.ranksStarted -= m_size;
                mod.ranksFinished = 0;
                unqueueExecute = true;
             } else {
                readyForReduce = false;
             }
         }
         break;
      }
   }

   if (readyForPrepare || readyForReduce) {
       vassert(!(readyForPrepare && readyForReduce));
       for (auto hub: receivingHubs) {
           message::Buffer buf(prog);
           buf.setBroadcast(false);
           buf.setDestRank(0);
           sendMessage(hub, buf);
       }
   }

   //CERR << prog.senderId() << " ready for prepare: " << readyForPrepare << ", reduce: " << readyForReduce << std::endl;
   for (auto output: portManager().getConnectedOutputPorts(prog.senderId())) {
      const Port::ConstPortSet *list = portManager().getConnectionList(output);
      for (const Port *destPort: *list) {
         if (!(destPort->flags() & Port::COMBINE)) {
            if (readyForPrepare)
               portManager().resetInput(destPort);
            if (readyForReduce)
               portManager().finishInput(destPort);
         }
      }
   }

   for (auto output: portManager().getConnectedOutputPorts(prog.senderId())) {
      const Port::ConstPortSet *list = portManager().getConnectionList(output);
      for (const Port *destPort: *list) {
         if (!(destPort->flags() & Port::COMBINE)) {
            bool allReadyForPrepare = true, allReadyForReduce = true;
            const int destId = destPort->getModuleID();
            auto allInputs = portManager().getConnectedInputPorts(destId);
            for (auto input: allInputs) {
               if (!portManager().isReset(input))
                  allReadyForPrepare = false;
               if (!portManager().isFinished(input))
                  allReadyForReduce = false;
            }
            //CERR << "exec prog: checking module " << destId << ":" << destPort->getName() << std::endl;
            auto it = m_stateTracker.runningMap.find(destId);
            vassert(it != m_stateTracker.runningMap.end());
            if (it->second.reducePolicy != message::ReducePolicy::Never || it->second.schedulingPolicy == message::SchedulingPolicy::LazyGang) {
               bool broadcast = handleOnMaster || it->second.reducePolicy!=message::ReducePolicy::Locally;
               if (allReadyForPrepare) {
                  for (auto input: allInputs) {
                     portManager().popReset(input);
                  }
                  //CERR << "Exec prepare 1" << std::endl;
                  if (isLocal(destId)) {
                     //CERR << "Exec prepare 2" << std::endl;
                     auto exec = message::Execute(message::Execute::Prepare, destId);
                     exec.setDestId(destId);
                     if (broadcast) {
                        //CERR << "Exec prepare 3" << std::endl;
                        if (m_rank == 0)
                           if (!Communicator::the().broadcastAndHandleMessage(exec))
                              return false;
                     } else {
                        //CERR << "Exec prepare 4" << std::endl;
                        handlePriv(exec);
                     }
                  }
               }
               if (allReadyForReduce) {
                  if (handleOnMaster && localSender) {
                     vassert(mod.ranksFinished==0);
                  }
                  // process all objects which are still in the queue befor calling reduce()
                  if (isLocal(destId)) {
                     if (it->second.schedulingPolicy == message::SchedulingPolicy::LazyGang) {
                         broadcast = true;
                         RunningMap::iterator i = runningMap.find(destId);
                         vassert(i != runningMap.end());
                         auto &destMod = i->second;

                         vassert(destMod.prepared);
                         vassert(!destMod.reduced);
                         if (m_rank == 0) {
                             if (destMod.objectCount.size() < getSize())
                                 destMod.objectCount.resize(getSize());
                             int maxNumObject = 0;
                             for (size_t r=0; r<destMod.objectCount.size(); ++r) {
                                 auto &c = destMod.objectCount[r];
                                 if (c > 0) {
                                     //CERR << "flushing " << c << " objects for rank " << r << ", module " << destId << std::endl;
                                 }
                                 if (c > maxNumObject)
                                     maxNumObject = c;
                                 c = 0;
                             }
                             for (int i=0; i<maxNumObject; ++i) {
                                 message::Execute exec(message::Execute::ComputeObject, destId);
                                 message::Buffer buf(exec);
                                 buf.setBroadcast(true);
                                 if (!Communicator::the().broadcastAndHandleMessage(buf))
                                     return false;
                             }
                             vassert(std::accumulate(destMod.objectCount.begin(), destMod.objectCount.end(), 0) == 0);
                         }
                     }
                  }
                  for (auto input: allInputs) {
                     portManager().popFinish(input);
                  }
                  if (isLocal(destId)) {
                     auto exec = message::Execute(message::Execute::Reduce, destId);
                     exec.setDestId(destId);
                     if (broadcast) {
                        if (m_rank == 0)
                           if (!Communicator::the().broadcastAndHandleMessage(exec))
                              return false;
                     } else {
                        handlePriv(exec);
                     }
                  }
               }
            }
         }
      }
   }

   if (unqueueExecute) {
       mod.processDelayed();
   }

   return true;
}

bool ClusterManager::handlePriv(const message::Busy &busy) {

   if (getRank() == 0) {
      int id = busy.senderId();
      auto &mod = runningMap[id];
      if (mod.busyCount == 0) {
         message::Buffer buf(busy);
         buf.setDestId(Id::UI);
         sendHub(buf);
      }
      ++mod.busyCount;
   } else {
      Communicator::the().forwardToMaster(busy);
   }
   return true;
}

bool ClusterManager::handlePriv(const message::Idle &idle) {

   if (getRank() == 0) {
      int id = idle.senderId();
      auto &mod = runningMap[id];
      --mod.busyCount;
      if (mod.busyCount == 0) {
         message::Buffer buf(idle);
         buf.setDestId(Id::UI);
         sendHub(buf);
      }
   } else {
      Communicator::the().forwardToMaster(idle);
   }
   return true;
}

bool ClusterManager::handlePriv(const message::ObjectReceived &objRecv) {

   sendMessage(objRecv.senderId(), objRecv);
   return true;
}


bool ClusterManager::handlePriv(const message::SetParameter &setParam) {

#ifdef DEBUG
   CERR << "SetParameter: " << setParam << std::endl;
#endif

   bool handled = true;
   int sender = setParam.senderId();
   int dest = setParam.destId();
   std::shared_ptr<Parameter> applied;
   if (message::Id::isModule(dest)) {
      // message to owning module
      auto param = getParameter(dest, setParam.getName());
      if (param) {
         applied.reset(param->clone());
         setParam.apply(applied);
      }
   } else if (message::Id::isModule(sender) && sender==setParam.getModule()) {
      // message from owning module
      auto param = getParameter(sender, setParam.getName());
      if (param) {
         setParam.apply(param);
      }
      if (dest == Id::ForBroadcast) {
          sendHub(setParam, dest);
      } else if (!Communicator::the().isMaster()) {
         sendAllOthers(sender, setParam, true);
      }
   }

   // update linked parameters
   if (Communicator::the().isMaster() && message::Id::isModule(dest)) {
      if (setParam.getModule() != setParam.senderId()) {

         const Port *port = portManager().findPort(dest, setParam.getName());
         if (port && applied) {
            ParameterSet conn = m_stateTracker.getConnectedParameters(*applied);

            for (ParameterSet::iterator it = conn.begin();
                  it != conn.end();
                  ++it) {
               const auto p = *it;
               if (p->module()==setParam.destId() && p->getName()==setParam.getName()) {
                  // don't update parameter which was set originally again
                  //continue;
               }
               message::SetParameter set(p->module(), p->getName(), applied);
               set.setDestId(p->module());
               set.setUuid(setParam.uuid());
               auto i = runningMap.find(p->module());
               if (i != runningMap.end()) {
                   auto &mod = i->second;
                   if (mod.haveDelayed())
                       mod.delay(set);
                   else
                       sendMessage(p->module(), set);
               }
            }
         } else {
#ifdef DEBUG
            CERR << " SetParameter ["
               << setParam.getModule() << ":" << setParam.getName()
               << "]: port not found" << std::endl;
#endif
         }
      }
   }

   return handled;
}

bool ClusterManager::handlePriv(const message::SetParameterChoices &setChoices) {
#ifdef DEBUG
   CERR << "SetParameterChoices: " << setChoices << std::endl;
#endif

   bool handled = true;
   int sender = setChoices.senderId();
   int dest = setChoices.destId();
   if (message::Id::isModule(dest)) {
      // message to owning module
      auto param = getParameter(dest, setChoices.getName());
      if (param) {
         setChoices.apply(param);
      }
   } else if (message::Id::isModule(sender)) {
      // message from owning module
      auto param = getParameter(sender, setChoices.getName());
      if (param) {
         setChoices.apply(param);
      }
      if (dest == Id::ForBroadcast) {
          sendHub(setChoices, Id::MasterHub);
      } else if (!Communicator::the().isMaster()) {
         sendAllOthers(sender, setChoices, true);
      }
   }

   return handled;
}

bool ClusterManager::handlePriv(const message::Barrier &barrier) {

   m_barrierActive = true;
   //sendHub(barrier);
   CERR << "Barrier [" << barrier.uuid() << "]" << std::endl;
   m_barrierUuid = barrier.uuid();
   if (checkBarrier(m_barrierUuid)) {
      barrierReached(m_barrierUuid);
   } else {
      sendAllLocal(barrier);
   }
   return true;
}

bool ClusterManager::handlePriv(const message::BarrierReached &barrReached) {

   vassert(m_barrierActive);
#ifdef BARRIER_DEBUG
   CERR << "BarrierReached [barrier " << barrReached.uuid() << ", module " << barrReached.senderId() << "]" << std::endl;
#endif

   if (barrReached.senderId() >= Id::ModuleBase) {
      vassert(isLocal(barrReached.senderId()));
      reachedSet.insert(barrReached.senderId());
      if (checkBarrier(m_barrierUuid)) {
         barrierReached(m_barrierUuid);
#ifdef BARRIER_DEBUG
      } else {
         CERR << "BARRIER: reached by " << reachedSet.size() << "/" << numRunning() << std::endl;
#endif
      }
   } else if (barrReached.senderId() == Id::MasterHub) {

      m_barrierActive = false;
   } else {
      CERR << "BARRIER: BarrierReached message from invalid sender " << barrReached.senderId() << std::endl;
   }

   return true;
}

bool ClusterManager::handlePriv(const message::SendText &text) {

   if (Communicator::the().isMaster()) {
      message::Buffer buf(text);
      buf.setDestId(Id::MasterHub);
      sendHub(buf);
   } else {
      sendHub(text);
   }
   return true;
}

bool ClusterManager::handlePriv(const message::RequestTunnel &tunnel) {

   using message::RequestTunnel;

   CERR << "RequestTunnel: remove=" << tunnel.remove() << " ";
   std::cerr << "host=" << tunnel.destHost() << " ";
   if (tunnel.destType() == RequestTunnel::IPv6) {
      std::cerr << "addr=" << tunnel.destAddrV6();
   } else if (tunnel.destType() == RequestTunnel::IPv4) {
      std::cerr << "addr=" << tunnel.destAddrV4();
   } else if (tunnel.destType() == RequestTunnel::Hostname) {
      std::cerr << "addr=" << tunnel.destHost();
   }
   std::cerr << std::endl;

   message::RequestTunnel tun(tunnel);
   tun.setDestId(Id::LocalHub);
   if (m_rank == 0) {
      if (!tunnel.remove() && tunnel.destType()==RequestTunnel::Unspecified) {
         CERR << "RequestTunnel: fill in local address" << std::endl;
         try {
            auto addr = Communicator::the().m_hubSocket.local_endpoint().address();
            if (addr.is_v6()) {
               tun.setDestAddr(addr.to_v6());
            } else if (addr.is_v4()) {
               tun.setDestAddr(addr.to_v4());
            }
         } catch (std::bad_cast &except) {
           CERR << "RequestTunnel: failed to convert local address to v6" << std::endl;
         }
      }
   }
   if (getRank() == 0)
      sendHub(tun);
   return true;
}

bool ClusterManager::handlePriv(const message::Ping &ping) {

   if (getRank() == 0) {
      message::Pong pong(ping);
      pong.setDestId(ping.senderId());
      sendHub(pong);
   }
   return true;
}

bool ClusterManager::quit() {

   if (!m_quitFlag)
      sendAllLocal(message::Kill(message::Id::Broadcast));

   // receive all ModuleExit messages from modules
   // retry for some time, modules that don't answer might have crashed
   CERR << "waiting for " << numRunning() << " modules to quit" << std::endl;

   if (m_size > 1)
      MPI_Barrier(MPI_COMM_WORLD);

   m_quitFlag = true;

   return numRunning()==0;
}

bool ClusterManager::quitOk() const {

   return m_quitFlag && numRunning()==0;
}

PortManager &ClusterManager::portManager() const {

   return *m_portManager;
}

std::vector<std::string> ClusterManager::getParameters(int id) const {

   return m_stateTracker.getParameters(id);
}

std::shared_ptr<Parameter> ClusterManager::getParameter(int id, const std::string &name) const {

   return m_stateTracker.getParameter(id, name);
}

void ClusterManager::queueMessage(const message::Message &msg) {

   m_messageQueue.emplace_back(msg);
#ifdef QUEUE_DEBUG
   CERR << "queueing " << msg.type() << ", now " << m_messageQueue.size() << " in queue" << std::endl;
#endif
}

void ClusterManager::replayMessages() {

   std::vector<message::Buffer> queue;
   std::swap(m_messageQueue, queue);
#ifdef QUEUE_DEBUG
   if (!queue.empty())
      CERR << "replaying " << queue.size() << " messages" << std::endl;
#endif
   for (const auto &m: queue) {
      Communicator::the().handleMessage(m);
   }
}

int ClusterManager::numRunning() const {

   int n = 0;
   for (auto &m: runningMap) {
      int state = m_stateTracker.getModuleState(m.first);
      if ((state & StateObserver::Initialized) && /* !(state & StateObserver::Killed) && */ !(state & StateObserver::Quit))
         ++n;
   }
   return n;
}

bool ClusterManager::isReadyForExecute(int modId) const {

   //CERR << "checking whether " << modId << " can be executed: ";

   auto i = runningMap.find(modId);
   if (i == runningMap.end()) {
      //std::cerr << "module " << modId << " not found" << std::endl;
      return false;
   }
   auto &mod = i->second;

   auto i2 = m_stateTracker.runningMap.find(modId);
   if (i2 == m_stateTracker.runningMap.end()) {
      //std::cerr << "module " << modId << " not found by state tracker" << std::endl;
      return false;
   }
   auto &modState = i2->second;

   if (modState.reducePolicy == message::ReducePolicy::Never) {
       //std::cerr << "reduce policy Never" << std::endl;
       return true;
   }

   if (!mod.reduced && mod.prepared) {
      vassert(mod.ranksFinished <= m_size);
      //std::cerr << "prepared & not reduced" << std::endl;
      return true;
   }

   //std::cerr << "prepared: " << mod.prepared << ", reduced: " << mod.reduced << std::endl;
   return false;
}

} // namespace vistle
