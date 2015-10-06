/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <util/sysdep.h>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <mpi.h>

#include <sys/types.h>

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <queue>
#include <future>

#include <core/message.h>
#include <core/messagequeue.h>
#include <core/object.h>
#include <core/shm.h>
#include <core/parameter.h>
#include <util/findself.h>

#include "clustermanager.h"
#include "datamanager.h"
#include "communicator.h"

//#define QUEUE_DEBUG

#ifdef NOHUB
#ifdef _WIN32
#define SPAWN_WITH_MPI
#else
#include <unistd.h>
#endif
#endif

#define CERR \
   std::cerr << "ClusterManager [" << m_rank << "/" << m_size << "] "

//#define DEBUG

namespace bi = boost::interprocess;

namespace vistle {

using message::Id;

void ClusterManager::Module::block(const message::Message &msg) {
   std::cerr << "BLOCK: " << msg << std::endl;
   blocked = true;
   blockers.push_back(message::Buffer(msg));
}

void ClusterManager::Module::unblock(const message::Message &msg) {
   std::cerr << "UNBLOCK: " << msg << " (blocked: " << blocked << ")" << std::endl;
   vassert(blocked);
   vassert(!blockers.empty());

   auto pred = [msg](const message::Buffer &buf) -> bool {
      return buf.uuid() == msg.uuid()
            && buf.type() == msg.type();
   };

   if (blocked) {
      if (pred(blockers.front())) {
         std::cerr << "UNBLOCK: found as frontmost blocker" << std::endl;
         blockers.pop_front();
         vassert(blockedMessages.front().uuid() == msg.uuid()
                && blockedMessages.front().type() == msg.type());
         blockedMessages.pop_front();
         sendQueue->send(msg);
         if (blockers.empty()) {
            std::cerr << "UNBLOCK: completely unblocked" << std::endl;
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
         std::cerr << "UNBLOCK: frontmost blocker: " << blockers.front() << std::endl;
         auto it = std::find_if(blockers.begin(), blockers.end(), pred);
         vassert(it != blockers.end());
         if (it != blockers.end()) {
            std::cerr << "UNBLOCK: found in blockers" << std::endl;
            blockers.erase(it);
         }
         it = std::find_if(blockedMessages.begin(), blockedMessages.end(), pred);
         vassert (it != blockedMessages.end());
         if (it != blockedMessages.end()) {
            std::cerr << "UNBLOCK: updating message" << std::endl;
            *it = message::Buffer(msg);
         }
      }
   }
}

bool ClusterManager::Module::send(const message::Message &msg) const {
   if (blocked) {
      blockedMessages.emplace_back(msg);
      return true;
   } else {
      return sendQueue->send(msg);
   }
}

bool ClusterManager::Module::update() const {
   return sendQueue->progress();
}


ClusterManager::ClusterManager(int r, const std::vector<std::string> &hosts)
: m_portManager(new PortManager(this))
, m_stateTracker(std::string("ClusterManager state rk")+boost::lexical_cast<std::string>(r), m_portManager)
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

bool ClusterManager::scanModules(const std::string &dir) {

#ifdef SCAN_MODULES_ON_HUB
    return true;
#else
   bool result = true;
   if (getRank() == 0) {
      AvailableMap availableModules;
      result = vistle::scanModules(dir, Communicator::the().hubId(), availableModules);
      for (auto &p: availableModules) {
         const auto &m = p.second;
         sendHub(message::ModuleAvailable(m.hub, m.name, m.path));
      }
   }
   return result;
#endif
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
   //CERR << "checkBarrier " << uuid << ": #local=" << numLocal << ", #reached=" << reachedSet.size() << std::endl;
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
      sendHub(m);
}

std::string ClusterManager::getModuleName(int id) const {

   return m_stateTracker.getModuleName(id);
}


bool ClusterManager::dispatch(bool &received) {

   bool done = false;

   // handle messages from modules closer to sink first
   // - should allow for objects to travel through the pipeline more quickly
   typedef StateTracker::Module Module;
   struct Comp {
      bool operator()(const Module &a, const Module &b) {
         return a.height > b.height;
      }
   };
   std::priority_queue<Module, std::vector<Module>, Comp> modules;
   for (auto m: m_stateTracker.runningMap) {
      const auto &mod = m.second;
      modules.emplace(mod);
   }

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
      if (Communicator::the().isMaster()) {
         buf.setDestId(Id::Broadcast);
         if (getRank() == 0)
            sendHub(buf);
      } else {
         int senderHub = message.senderId();
         if (senderHub >= Id::ModuleBase)
            senderHub = m_stateTracker.getHub(senderHub);
         if (senderHub == Communicator::the().hubId()) {
            buf.setDestId(Id::Broadcast);
            if (getRank() == 0)
               sendHub(buf);
         }
      }
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

   return Communicator::the().sendHub(message);
}

bool ClusterManager::sendHub(const message::Message &message, int destHub) const {

   if (destHub == Id::Broadcast) {
      return Communicator::the().sendHub(message);
   } else {
      message::Buffer buf(message);
      buf.setDestId(destHub);
      return Communicator::the().sendHub(buf);
   }
}

bool ClusterManager::sendMessage(const int moduleId, const message::Message &message, int destRank) const {

   const int hub = m_stateTracker.getHub(moduleId);

   if (hub == Communicator::the().hubId()) {
      //std::cerr << "local send to " << moduleId << ": " << message << std::endl;
      RunningMap::const_iterator it = runningMap.find(moduleId);
      if (it == runningMap.end()) {
         CERR << "sendMessage: module " << moduleId << " not found" << std::endl;
         std::cerr << "  message: " << message << std::endl;
         return false;
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

bool ClusterManager::handle(const message::Message &message) {

   using namespace vistle::message;

   switch (message.type()) {
      case Message::CONNECT:
      case Message::DISCONNECT:
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
      if (message.senderId() != hubId && senderHub == hubId) {
         //CERR << "BC: " << message << std::endl;
         if (getRank() == 0)
            sendHub(message);
      }
      if (message.typeFlags() & BroadcastModule) {
         sendAllLocal(message);
      }
   }
   if (message.destId() >= Id::ModuleBase) {
      if (destHub == hubId) {
         //CERR << "module: " << message << std::endl;
         if (message.type() != message::Message::EXECUTE) {
            return sendMessage(message.destId(), message);
         }
      } else {
         return sendHub(message);
      }
   }

   switch (message.type()) {

      case Message::IDENTIFY: {

         const Identify &id = static_cast<const message::Identify &>(message);
         vassert(id.identity() == Identify::UNKNOWN);
         if (getRank() == 0) {
            sendHub(Identify(Identify::MANAGER));
            auto avail = availableModules();
            for(const auto &mod: avail) {
               sendHub(message::ModuleAvailable(hubId, mod.name, mod.path));
            }
         }
         break;
      }

      case message::Message::QUIT: {

         result = false;
         break;
      }

      case message::Message::TRACE: {
         const Trace &trace = static_cast<const Trace &>(message);
         result = handlePriv(trace);
         break;
      }

      case message::Message::SPAWN: {

         const message::Spawn &spawn = static_cast<const message::Spawn &>(message);
         result = handlePriv(spawn);
         break;
      }

      case message::Message::CONNECT: {

         const message::Connect &connect = static_cast<const message::Connect &>(message);
         result = handlePriv(connect);
         break;
      }

      case message::Message::DISCONNECT: {

         const message::Disconnect &disc = static_cast<const message::Disconnect &>(message);
         result = handlePriv(disc);
         break;
      }

      case message::Message::MODULEEXIT: {

         const message::ModuleExit &moduleExit = static_cast<const message::ModuleExit &>(message);
         result = handlePriv(moduleExit);
         break;
      }

      case message::Message::EXECUTE: {

         const message::Execute &exec = static_cast<const message::Execute &>(message);
         result = handlePriv(exec);
         break;
      }

      case message::Message::ADDOBJECT: {

         const message::AddObject &m = static_cast<const message::AddObject &>(message);
         result = handlePriv(m);
         break;
      }

      case message::Message::ADDOBJECTCOMPLETED: {
         const message::AddObjectCompleted &m = static_cast<const message::AddObjectCompleted &>(message);
         result = handlePriv(m);
         break;
      }

      case message::Message::EXECUTIONPROGRESS: {

         const message::ExecutionProgress &prog = static_cast<const message::ExecutionProgress &>(message);
         result = handlePriv(prog);
         break;
      }

      case message::Message::BUSY: {

         const message::Busy &busy = static_cast<const message::Busy &>(message);
         result = handlePriv(busy);
         break;
      }

      case message::Message::IDLE: {

         const message::Idle &idle = static_cast<const message::Idle &>(message);
         result = handlePriv(idle);
         break;
      }

      case message::Message::OBJECTRECEIVED: {
         const message::ObjectReceived &m = static_cast<const message::ObjectReceived &>(message);
         result = handlePriv(m);
         break;
      }

      case message::Message::SETPARAMETER: {

         const message::SetParameter &m = static_cast<const message::SetParameter &>(message);
         result = handlePriv(m);
         break;
      }

      case Message::SETPARAMETERCHOICES: {
         const message::SetParameterChoices &m = static_cast<const message::SetParameterChoices &>(message);
         result = handlePriv(m);
         break;
      }

      case message::Message::BARRIER: {

         const message::Barrier &m = static_cast<const message::Barrier &>(message);
         result = handlePriv(m);
         break;
      }

      case message::Message::BARRIERREACHED: {

         const message::BarrierReached &m = static_cast<const message::BarrierReached &>(message);
         result = handlePriv(m);
         break;
      }

      case message::Message::SENDTEXT: {
         const message::SendText &m = static_cast<const message::SendText &>(message);
         result = handlePriv(m);
         break;
      }

      case message::Message::REQUESTTUNNEL: {
         const message::RequestTunnel &m = static_cast<const message::RequestTunnel &>(message);
         result = handlePriv(m);
         break;
      }

      case Message::ADDHUB:
      case Message::REMOVESLAVE:
      case Message::STARTED:
      case Message::ADDPORT:
      case Message::ADDPARAMETER:
      case Message::MODULEAVAILABLE:
      case Message::REPLAYFINISHED:
      case Message::REDUCEPOLICY:
      case Message::OBJECTRECEIVEPOLICY:
      case Message::PING:
      case Message::PONG:
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

   if (trace.module() >= Id::ModuleBase) {
      sendMessage(trace.module(), trace);
   } else if (trace.module() == Id::Broadcast) {
      sendAll(trace);
   }
#if 0
   if (trace.module() == Id::Broadcast || trace.module() == Communicator::the().hubId()) {
      if (trace.on())
         m_traceMessages = trace.messageType();
      else
         m_traceMessages = message::Message::INVALID;
   }
#endif
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
   const Port *from = portManager().getPort(modFrom, portFrom);
   const Port *to = portManager().getPort(modTo, portTo);

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
   
   m_stateTracker.handle(d);
   if (portManager().removeConnection(modFrom, portFrom, modTo, portTo)) {

      if (m_stateTracker.getHub(modFrom) == Communicator::the().hubId())
         sendMessage(modFrom, d);
      if (m_stateTracker.getHub(modTo) == Communicator::the().hubId())
         sendMessage(modTo, d);
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

   int mod = moduleExit.senderId();
   sendAllOthers(mod, moduleExit, true);

   if (!moduleExit.isForwarded()) {

      if (isLocal(moduleExit.senderId())) {
         RunningMap::iterator it = runningMap.find(mod);
         if (it == runningMap.end()) {
            CERR << " Module [" << mod << "] quit, but not found in running map" << std::endl;
            return true;
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
         CERR << " Module [" << mod << "] not found in map" << std::endl;
      }
   }
   {
      ModuleSet::iterator it = reachedSet.find(mod);
      if (it != reachedSet.end())
         reachedSet.erase(it);
   }

   if (m_barrierActive && checkBarrier(m_barrierUuid))
      barrierReached(m_barrierUuid);

   return true;
}

bool ClusterManager::handlePriv(const message::Execute &exec) {

   if (exec.senderId() >= Id::ModuleBase) {

      if (getRank() == 0)
         sendHub(exec);
   } else {

      vassert (exec.getModule() >= Id::ModuleBase);
      RunningMap::iterator i = runningMap.find(exec.getModule());
      if (i != runningMap.end()) {
         i->second.send(exec);
         switch(exec.what()) {
         case message::Execute::Prepare:
             vassert(!i->second.prepared);
             vassert(i->second.reduced);
             i->second.prepared = true;
             i->second.reduced = false;
             CERR << "sent prepare to " << exec.getModule() << ", checking for execution" << std::endl;
             checkExecuteObject(exec.getModule());
             break;
         case message::Execute::Reduce:
             vassert(i->second.prepared);
             vassert(!i->second.reduced);
             i->second.prepared = false;
             i->second.reduced = true;
             break;
         case message::Execute::ComputeExecute:
             vassert(!i->second.prepared);
             vassert(i->second.reduced);
             i->second.reduced = true;
             i->second.prepared = false;
             break;
         case message::Execute::ComputeObject:
             break;
         }
      }
   }

   return true;
}

bool ClusterManager::handlePriv(const message::AddObject &addObj, bool synthesized) {

   const bool localAdd = isLocal(addObj.senderId());
   CERR << "ADDOBJECT: " << addObj << ", local=" << localAdd << ", synthesized=" << synthesized << std::endl;
   Object::const_ptr obj;

   if (localAdd) {
      obj = addObj.takeObject();
      CERR << "ADDOBJECT: local, name=" << obj->getName() << ", refcount=" << obj->refcount() << std::endl;
   } else {

      int destRank = -1;
      if (getRank() == 0) {
         int block = addObj.meta().block();
         if (block >= 0) {
            destRank = block % getSize();
         }
      }
      CERR << "ADDOBJECT from remote, handling on rank " << destRank << std::endl;
      if (destRank == getRank() || (getRank() == 0 && destRank == -1)) {
         obj = addObj.takeObject();
         if (!obj) {
            vassert(!synthesized);
            CERR << "AddObject: have to request " << addObj.objectName() << std::endl;
            Communicator::the().dataManager().requestObject(addObj, addObj.objectName(), []() -> void { /* FIXME */ });
         }
      } else {
          return true;
      }
   }

   vassert(!(synthesized && localAdd));
   if (synthesized || localAdd) {
      vassert(obj);
   }
   vassert(!obj || (obj->refcount() >= 1 && (localAdd || synthesized)));

   Port *port = portManager().getPort(addObj.senderId(), addObj.getSenderPort());
   if (!port) {
      CERR << "AddObject [" << addObj.objectName() << "] to port ["
         << addObj.getSenderPort() << "] of [" << addObj.senderId() << "]: port not found" << std::endl;
      vassert(port);
      return true;
   }
   const Port::PortSet *list = portManager().getConnectionList(port);
   if (!list) {
      CERR << "AddObject [" << addObj.objectName() << "] to port ["
         << addObj.getSenderPort() << "] of [" << addObj.senderId() << "]: connection list not found" << std::endl;
      vassert(list);
      return true;
   }

   std::set<int> receivingHubs; // make sure that message is only sent once per remote hub
   for (const Port *destPort: *list) {
      int destId = destPort->getModuleID();
      message::AddObject a(addObj);
      a.takeObject();

      if (!isLocal(destId)) {
         if (localAdd) {
            // if object was generated locally, forward message to remote hubs with connected modules
            const int hub = m_stateTracker.getHub(destId);
            a.setDestId(hub);
            if (receivingHubs.find(hub) == receivingHubs.end()) {
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
            it->second.unblock(add);
         }
         continue;
      }

      sendMessage(destId, a);
      portManager().addObject(destPort);

      if (!checkExecuteObject(destId))
          return false;

      auto it = m_stateTracker.runningMap.find(destId);
      if (it == m_stateTracker.runningMap.end()) {
         CERR << "port connection to module that is not running" << std::endl;
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

   CERR << "checking whether " << destId << " can be executed" << std::endl;
   if (!isReadyForExecute(destId))
       return true;
   CERR << "CHK ready for exec" << std::endl;

   for (const auto input: portManager().getConnectedInputPorts(destId)) {
      if (!portManager().hasObject(input)) {
         return true;
      }
   }
   CERR << "CHK all ready" << std::endl;

   for (const auto input: portManager().getConnectedInputPorts(destId)) {
      portManager().popObject(input);
   }

   auto it = m_stateTracker.runningMap.find(destId);
   if (it == m_stateTracker.runningMap.end()) {
      CERR << "port connection to module that is not running" << std::endl;
      vassert("port connection to module that is not running" == 0);
      return true;
   }
   auto &destMod = it->second;
   message::Execute c(message::Execute::ComputeObject, destId);
   //c.setUuid(addObj.uuid());
   if (destMod.schedulingPolicy == message::SchedulingPolicy::Single) {
      sendMessage(destId, c);
   } else {
      c.setAllRanks(true);
      if (!Communicator::the().broadcastAndHandleMessage(c))
         return false;
   }

   return true;
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
   auto &mod = i->second;

   auto i2 = m_stateTracker.runningMap.find(prog.senderId());
   if (i2 == m_stateTracker.runningMap.end()) {
      CERR << "module " << prog.senderId() << " not found" << std::endl;
      return false;
   }
   auto &modState = i2->second;

   // initiate reduction if requested by module

   bool handleOnMaster = false;
   // forward message to remote hubs...
   std::set<int> receivingHubs; // ...but make sure that message is only sent once per remote hub
   if (localSender) {
      for (auto output: portManager().getConnectedOutputPorts(prog.senderId())) {
         const Port::PortSet *list = portManager().getConnectionList(output);
         for (const Port *destPort: *list) {
            int destId = destPort->getModuleID();
            if (!isLocal(destId)) {
               int hub = m_stateTracker.getHub(destId);
               receivingHubs.insert(hub);
            }
            const auto it = m_stateTracker.runningMap.find(destId);
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
      CERR << "exec progr: forward to master" << std::endl;
      return Communicator::the().forwardToMaster(prog);
   }

   bool readyForPrepare = false, readyForReduce = false;
   CERR << "ExecutionProgress " << prog.stage() << " received from " << prog.senderId() << ":" << prog.rank() << std::endl;
   switch (prog.stage()) {
      case message::ExecutionProgress::Start: {
         readyForPrepare = true;
         if (handleOnMaster && localSender) {
             vassert(mod.ranksFinished == 0);
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
                   CERR << "mismatch: m_size=" << m_size << ", started=" << mod.ranksStarted << std::endl;
                }
                vassert(mod.ranksStarted == m_size);
                mod.ranksFinished = 0;
                mod.ranksStarted = 0;
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

   CERR << prog.senderId() << " ready for prepare: " << readyForPrepare << ", reduce: " << readyForReduce << std::endl;
   for (auto output: portManager().getConnectedOutputPorts(prog.senderId())) {
      const Port::PortSet *list = portManager().getConnectionList(output);
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
      const Port::PortSet *list = portManager().getConnectionList(output);
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
            CERR << "exec prog: checking module " << destId << ":" << destPort->getName() << std::endl;
            auto it = m_stateTracker.runningMap.find(destId);
            vassert(it != m_stateTracker.runningMap.end());
            if (it->second.reducePolicy != message::ReducePolicy::Never) {
               const bool broadcast = it->second.reducePolicy!=message::ReducePolicy::Locally;
               if (allReadyForPrepare) {
                  for (auto input: allInputs) {
                     portManager().popReset(input);
                  }
                  CERR << "Exec prepare 1" << std::endl;
                  if (isLocal(destId)) {
                     CERR << "Exec prepare 2" << std::endl;
                     auto exec = message::Execute(message::Execute::Prepare, destId);
                     exec.setDestId(destId);
                     if (broadcast) {
                        CERR << "Exec prepare 3" << std::endl;
                        if (m_rank == 0)
                           if (!Communicator::the().broadcastAndHandleMessage(exec))
                              return false;
                     } else {
                        CERR << "Exec prepare 4" << std::endl;
                        sendMessage(destId, exec);
                        checkExecuteObject(destId);
                     }
                  }
               }
               if (allReadyForReduce) {
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
                        sendMessage(destId, exec);
                     }
                  }
               }
            }
         }
      }
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
   boost::shared_ptr<Parameter> applied;
   if (message::Id::isModule(dest)) {
      // message to owning module
      auto param = getParameter(dest, setParam.getName());
      if (param) {
         applied.reset(param->clone());
         setParam.apply(applied);
      }
   } else if (message::Id::isModule(sender)) {
      // message from owning module
      auto param = getParameter(sender, setParam.getName());
      if (param) {
         setParam.apply(param);
      }
   }

   if (setParam.isReply()) {
      sendAllOthers(setParam.senderId(), setParam, true);
   }

   // update linked parameters
   if (Communicator::the().isMaster() && message::Id::isModule(dest)) {
      if (!setParam.isReply()) {

         Port *port = portManager().getPort(dest, setParam.getName());
         if (port && applied) {
            ParameterSet conn = m_stateTracker.getConnectedParameters(*applied);

            for (ParameterSet::iterator it = conn.begin();
                  it != conn.end();
                  ++it) {
               const auto p = *it;
               if (p->module()==setParam.destId() && p->getName()==setParam.getName()) {
                  // don't update parameter which was set originally again
                  continue;
               }
               message::SetParameter set(p->getName(), applied);
               set.setDestId(p->module());
               set.setUuid(setParam.uuid());
               sendMessage(p->module(), set);
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
      sendAllOthers(setChoices.senderId(), setChoices, true);
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
#ifdef DEBUG
   CERR << "BarrierReached [barrier " << barrReached.uuid() << ", module " << barrReached.senderId() << "]" << std::endl;
#endif

   if (barrReached.senderId() >= Id::ModuleBase) {
      reachedSet.insert(barrReached.senderId());
      if (checkBarrier(m_barrierUuid)) {
         barrierReached(m_barrierUuid);
#ifdef DEBUG
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

boost::shared_ptr<Parameter> ClusterManager::getParameter(int id, const std::string &name) const {

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
      if ((state & StateObserver::Initialized) && !(state & StateObserver::Killed) && !(state & StateObserver::Quit))
         ++n;
   }
   return n;
}

bool ClusterManager::isReadyForExecute(int modId) const {

   auto i = runningMap.find(modId);
   if (i == runningMap.end()) {
      CERR << "module " << modId << " not found" << std::endl;
      return false;
   }
   auto &mod = i->second;

   auto i2 = m_stateTracker.runningMap.find(modId);
   if (i2 == m_stateTracker.runningMap.end()) {
      CERR << "module " << modId << " not found by state tracker" << std::endl;
      return false;
   }
   auto &modState = i2->second;

   if (modState.reducePolicy == message::ReducePolicy::Never)
       return true;

   if (!mod.reduced && mod.prepared)
      return true;

   return false;
}

} // namespace vistle
