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

#include <core/message.h>
#include <core/messagequeue.h>
#include <core/object.h>
#include <core/shm.h>
#include <core/parameter.h>
#include <util/findself.h>

#include "clustermanager.h"
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

int ClusterManager::getRank() const {

   return m_rank;
}

int ClusterManager::getSize() const {

   return m_size;
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
               recv = mq->tryReceive(buf.msg);
            } catch (boost::interprocess::interprocess_exception &ex) {
               CERR << "receive mq " << ex.what() << std::endl;
               exit(-1);
            }
         }

         if (recv) {
            received = true;
            if (!Communicator::the().handleMessage(buf.msg))
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
         buf.msg.setDestId(Id::Broadcast);
         sendHub(buf.msg);
      } else {
         int senderHub = message.senderId();
         if (senderHub >= Id::ModuleBase)
            senderHub = m_stateTracker.getHub(senderHub);
         if (senderHub == Communicator::the().hubId()) {
            buf.msg.setDestId(Id::Broadcast);
            sendHub(buf.msg);
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

bool ClusterManager::sendHub(const message::Message &message) const {

   return Communicator::the().sendHub(message);
}

bool ClusterManager::sendMessage(const int moduleId, const message::Message &message) const {

   const int hub = m_stateTracker.getHub(moduleId);

   if (hub == Communicator::the().hubId()) {
      //std::cerr << "local send to " << moduleId << ": " << message << std::endl;
      RunningMap::const_iterator it = runningMap.find(moduleId);
      if (it == runningMap.end()) {
         CERR << "sendMessage: module " << moduleId << " not found" << std::endl;
         std::cerr << "  message: " << message << std::endl;
         return false;
      }

      auto &mod = it->second;
      mod.send(message);
   } else {
      std::cerr << "remote send to " << moduleId << ": " << message << std::endl;
      message::Buffer buf(message);
      buf.msg.setDestId(moduleId);
      sendHub(message);
   }

   return true;
}

bool ClusterManager::handle(const message::Message &message) {

   using namespace vistle::message;

   m_stateTracker.handle(message);

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
         sendHub(message);
      }
      if (message.typeFlags() & BroadcastModule) {
         sendAllLocal(message);
      }
   }
   if (message.destId() >= Id::ModuleBase) {
      if (destHub == hubId) {
         //CERR << "module: " << message << std::endl;
         return sendMessage(message.destId(), message);
      } else {
         return sendHub(message);
      }
   }

   switch (message.type()) {

      case Message::IDENTIFY: {

         const Identify &id = static_cast<const message::Identify &>(message);
         vassert(id.identity() == Identify::UNKNOWN);
         sendHub(Identify(Identify::MANAGER));
         auto avail = availableModules();
         for(const auto &mod: avail) {
            sendHub(message::ModuleAvailable(hubId, mod.name, mod.path));
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
      case Message::SETPARAMETERCHOICES:
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
   if (spawn.destId() != Communicator::the().hubId())
      return true;

   int newId = spawn.spawnId();
   const std::string idStr = boost::lexical_cast<std::string>(newId);

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
   sendHub(prep);

   // inform newly started module about current parameter values of other modules
   auto state = m_stateTracker.getState();
   for (const auto &m: state) {
      sendMessage(newId, m.msg);
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

   m_stateTracker.handle(c);
   if (portManager().addConnection(modFrom, portFrom, modTo, portTo)) {
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
   sendAllOthers(mod, moduleExit);

   if (!moduleExit.isForwarded()) {

      RunningMap::iterator it = runningMap.find(mod);
      if (it == runningMap.end()) {
         CERR << " Module [" << mod << "] quit, but not found in running map" << std::endl;
         return true;
      }
      if (m_rank == 0) {
         message::ModuleExit exit = moduleExit;
         exit.setForwarded();
         sendHub(exit);
         if (!Communicator::the().broadcastAndHandleMessage(exit))
            return false;
      }
      return true;
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

      sendHub(exec);
   } else {

      vassert (exec.getModule() >= Id::ModuleBase);
      RunningMap::iterator i = runningMap.find(exec.getModule());
      if (i != runningMap.end()) {
         i->second.send(exec);
      }
   }

   return true;
}

bool ClusterManager::handlePriv(const message::AddObject &addObj) {

   Object::const_ptr obj = addObj.takeObject();
   vassert(obj->refcount() >= 1);
#if 0
   std::cerr << "Module " << addObj.senderId() << ": "
      << "AddObject " << addObj.getHandle() << " (" << obj->getName() << ")"
      << " ref " << obj->refcount()
      << " to port " << addObj.getPortName() << std::endl;
#endif

   Port *port = portManager().getPort(addObj.senderId(), addObj.getPortName());
   if (!port) {
      CERR << "AddObject ["
         << addObj.getHandle() << "] to port ["
         << addObj.getPortName() << "] of [" << addObj.senderId() << "]: port not found" << std::endl;
      vassert(port);
      return true;
   }
   const Port::PortSet *list = portManager().getConnectionList(port);
   if (!list) {
      CERR << "AddObject ["
         << addObj.getHandle() << "] to port ["
         << addObj.getPortName() << "] of [" << addObj.senderId() << "]: connection list not found" << std::endl;
      vassert(list);
      return true;
   }

   for (const Port *destPort: *list) {

      int destId = destPort->getModuleID();

      message::AddObject a(addObj.getSenderPort(), destPort->getName(), obj);
      a.setSenderId(addObj.senderId());
      a.setUuid(addObj.uuid());
      a.setRank(addObj.rank());
      sendMessage(destId, a);

      portManager().addObject(destPort);

      auto it = m_stateTracker.runningMap.find(destId);
      if (it == m_stateTracker.runningMap.end()) {
         CERR << "port connection to module that is not running" << std::endl;
         vassert("port connection to module that is not running" == 0);
         continue;
      }
      auto &destMod = it->second;

      bool allReady = true;
      for (const auto input: portManager().getConnectedInputPorts(destId)) {
         if (!portManager().hasObject(input)) {
            allReady = false;
            break;
         }
      }

      if (allReady) {

         for (const auto input: portManager().getConnectedInputPorts(destId)) {
            portManager().popObject(input);
         }

         message::Execute c(message::Execute::ComputeObject, destId);
         c.setUuid(addObj.uuid());
         if (destMod.schedulingPolicy == message::SchedulingPolicy::Single) {
            sendMessage(destId, c);
         } else {
            c.setAllRanks(true);
            if (!Communicator::the().broadcastAndHandleMessage(c))
               return false;
         }

         if (destMod.objectPolicy == message::ObjectReceivePolicy::NotifyAll
               || destMod.objectPolicy == message::ObjectReceivePolicy::Distribute) {
            message::ObjectReceived recv(addObj.getSenderPort(), addObj.getPortName(), obj);
            recv.setUuid(addObj.uuid());
            recv.setSenderId(destId);

            if (!Communicator::the().broadcastAndHandleMessage(recv))
               return false;
         }
      }
   }

   return true;
}

bool ClusterManager::handlePriv(const message::ExecutionProgress &prog) {

   RunningMap::iterator i = runningMap.find(prog.senderId());
   if (i == runningMap.end()) {
      return false;
   }

   auto i2 = m_stateTracker.runningMap.find(prog.senderId());
   if (i2 == m_stateTracker.runningMap.end()) {
      return false;
   }

   // initiate reduction if requested by module
   auto &mod = i->second;
   auto &mod2 = i2->second;

   const bool handleOnMaster = mod2.reducePolicy != message::ReducePolicy::Never;
   if (handleOnMaster && m_rank != 0) {
      return Communicator::the().forwardToMaster(prog);
   }

   bool readyForPrepare = false, readyForReduce = false;
   bool result = true;
   //std::cerr << "ExecutionProgress " << prog.stage() << " received from " << prog.senderId() << "/" << prog.rank() << std::endl;
   switch (prog.stage()) {
      case message::ExecutionProgress::Start: {
         readyForPrepare = true;
         if (handleOnMaster) {
            vassert(mod.ranksFinished < m_size);
            ++mod.ranksStarted;
            readyForPrepare = mod.ranksStarted==m_size;
         }
         break;
      }

      case message::ExecutionProgress::Finish: {
         readyForReduce = true;
         if (handleOnMaster) {
            ++mod.ranksFinished;
            if (mod.ranksFinished == m_size) {
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

   //std::cerr << prog.senderId() << " ready for prepare: " << readyForPrepare << ", reduce: " << readyForReduce << std::endl;
   for (auto output: portManager().getConnectedOutputPorts(prog.senderId())) {
      const Port::PortSet *list = portManager().getConnectionList(output);
      for (const Port *destPort: *list) {
         if (readyForPrepare)
            portManager().resetInput(destPort);
         if (readyForReduce)
            portManager().finishInput(destPort);
      }
   }

   for (auto output: portManager().getConnectedOutputPorts(prog.senderId())) {
      const Port::PortSet *list = portManager().getConnectionList(output);
      for (const Port *destPort: *list) {
         bool allReadyForPrepare = true, allReadyForReduce = true;
         int destId = destPort->getModuleID();
         auto allInputs = portManager().getConnectedInputPorts(destId);
         for (auto input: allInputs) {
            if (!portManager().isReset(input))
               allReadyForPrepare = false;
            if (!portManager().isFinished(input))
               allReadyForReduce = false;
         }
         if (allReadyForPrepare) {
            for (auto input: allInputs) {
               portManager().popReset(input);
            }
            auto exec = message::Execute(message::Execute::Prepare, destId);
            if (handleOnMaster) {
               if (!Communicator::the().broadcastAndHandleMessage(exec))
                  return false;
            } else {
               sendMessage(destId, exec);
            }
         }
         if (allReadyForReduce) {
            for (auto input: allInputs) {
               portManager().popFinish(input);
            }
            auto exec = message::Execute(message::Execute::Reduce, destId);
            if (handleOnMaster) {
               if (!Communicator::the().broadcastAndHandleMessage(exec))
                  return false;
            } else {
               sendMessage(destId, exec);
            }
         }
      }
   }

   return result;
}

bool ClusterManager::handlePriv(const message::Busy &busy) {

   if (getRank() == 0) {
      int id = busy.senderId();
      auto &mod = runningMap[id];
      if (mod.busyCount == 0) {
         message::Buffer buf(busy);
         buf.msg.setDestId(Id::UI);
         sendHub(buf.msg);
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
         buf.msg.setDestId(Id::UI);
         sendHub(buf.msg);
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
   CERR << "SetParameter: sender=" << setParam.senderId() << ", module=" << setParam.getModule() << ", name=" << setParam.getName() << std::endl;
#endif

   bool handled = true;
   auto param = getParameter(setParam.getModule(), setParam.getName());
   boost::shared_ptr<Parameter> applied;
   if (param) {
      applied.reset(param->clone());
      setParam.apply(applied);
   }
   if (setParam.senderId() != setParam.getModule()) {
      // message to owning module
      RunningMap::iterator i = runningMap.find(setParam.getModule());
      if (i != runningMap.end() && param)
         sendMessage(setParam.getModule(), setParam);
      else
         handled = false;
   } else {
      // notification by owning module about a changed parameter
      if (param) {
         setParam.apply(param);
      } else {
         CERR << "no such parameter: module id=" << setParam.getModule() << ", name=" << setParam.getName() << std::endl;
      }

      // let all modules know that a parameter was changed
      sendAllOthers(setParam.senderId(), setParam);
   }

   if (!setParam.isReply()) {

      // update linked parameters
      if (Communicator::the().isMaster()) {
         Port *port = portManager().getPort(setParam.getModule(), setParam.getName());
         if (port && applied) {
            ParameterSet conn = m_stateTracker.getConnectedParameters(*param);

            for (ParameterSet::iterator it = conn.begin();
                  it != conn.end();
                  ++it) {
               const auto p = *it;
               if (p->module()==setParam.getModule() && p->getName()==setParam.getName()) {
                  // don't update parameter which was set originally again
                  continue;
               }
               message::SetParameter set(p->module(), p->getName(), applied);
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

   if (m_rank == 0) {
      if (Communicator::the().isMaster()) {
         message::Buffer buf(text);
         buf.msg.setDestId(Id::MasterHub);
         sendHub(buf.msg);
      } else {
         sendHub(text);
      }
   } else {
      Communicator::the().forwardToMaster(text);
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
      Communicator::the().handleMessage(m.msg);
   }
}

int ClusterManager::numRunning() const {

   int n = 0;
   for (auto &m: runningMap) {
      int state = m_stateTracker.getModuleState(m.first);
      if ((state & StateObserver::Initialized) && !(state & StateObserver::Killed))
         ++n;
   }
   return n;
}

} // namespace vistle
