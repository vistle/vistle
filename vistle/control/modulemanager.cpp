/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <boost/foreach.hpp>

#include <mpi.h>

#include <sys/types.h>

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <core/message.h>
#include <core/messagequeue.h>
#include <core/object.h>
#include <core/shm.h>
#include <core/parameter.h>
#include <util/findself.h>

#include "modulemanager.h"
#include "communicator.h"

#define CERR \
   std::cerr << "mod mgr [" << m_rank << "/" << m_size << "] "

//#define DEBUG

namespace bi = boost::interprocess;

namespace vistle {

ModuleManager::ModuleManager(int argc, char *argv[], int r, const std::vector<std::string> &hosts)
: m_portManager(this)
, m_stateTracker(&m_portManager)
, m_rank(r)
, m_size(hosts.size())
, m_hosts(hosts)
, m_moduleCounter(0)
, m_executionCounter(0)
, m_barrierCounter(0)
, m_activeBarrier(-1)
, m_reachedBarriers(-1)
{

   message::DefaultSender::init(0, m_rank);

   m_bindir = getbindir(argc, argv);
}

int ModuleManager::getRank() const {

   return m_rank;
}

int ModuleManager::getSize() const {

   return m_size;
}

int ModuleManager::newModuleID() {
   ++m_moduleCounter;

   return m_moduleCounter;
}

int ModuleManager::newExecutionCount() {
   ++m_executionCounter;

   return m_executionCounter;
}

void ModuleManager::resetModuleCounter() {

   m_moduleCounter = 0;
}

int ModuleManager::getBarrierCounter() {

   ++m_barrierCounter;
   return m_barrierCounter;
}

boost::mutex &ModuleManager::barrierMutex() {

   return m_barrierMutex;
}

boost::condition_variable &ModuleManager::barrierCondition() {

   return m_barrierCondition;
}

bool ModuleManager::checkBarrier(int id) const {

   int numLocal = 0;
   for (const auto &m: runningMap) {
      if (m.second.local)
         ++numLocal;
   }
   //CERR << "checkBarrier " << id << ": #local=" << numLocal << ", #reached=" << reachedSet.size() << std::endl;
   if (reachedSet.size() == numLocal)
      return true;

   return false;
}

void ModuleManager::barrierReached(int id) {

   assert(id >= 0);
   MPI_Barrier(MPI_COMM_WORLD);
   m_reachedBarriers = id;
   m_activeBarrier = -1;
   reachedSet.clear();
   CERR << "Barrier [" << id << "] reached" << std::endl;
   m_barrierCondition.notify_all();
}

std::vector<int> ModuleManager::getRunningList() const {

   return m_stateTracker.getRunningList();
}

std::vector<int> ModuleManager::getBusyList() const {

   return m_stateTracker.getBusyList();
}

std::string ModuleManager::getModuleName(int id) const {

   return m_stateTracker.getModuleName(id);
}


bool ModuleManager::dispatch(bool &received) {

   bool done = false;

   // handle messages from modules
   for(RunningMap::iterator next, it = runningMap.begin();
         it != runningMap.end();
         it = next) {
      next = it;
      ++next;

      const int modId = it->first;
      const Module &mod = it->second;

      // keep messages from modules that have already reached a barrier on hold
      if (reachedSet.find(modId) != reachedSet.end())
         continue;

      if (mod.local) {
         bi::message_queue &mq = mod.recvQueue->getMessageQueue();

         bool recv = false;
         if (!Communicator::the().tryReceiveAndHandleMessage(mq, recv)) {
            if (recv)
               done = true;
         }

         if (recv)
            received = true;
      }
   }

   return !done;
}

bool ModuleManager::sendAll(const message::Message &message) const {

   // -1 is an invalid module id
   return sendAllOthers(-1, message);
}

bool ModuleManager::sendAllOthers(int excluded, const message::Message &message) const {

   // handle messages to modules
   for(RunningMap::const_iterator next, it = runningMap.begin();
         it != runningMap.end();
         it = next) {
      next = it;
      ++next;

      const int modId = it->first;
      if (modId == excluded)
         continue;
      const Module &mod = it->second;

      if (mod.local)
         mod.sendQueue->getMessageQueue().send(&message, message.size(), 0);
   }

   return true;
}

bool ModuleManager::sendUi(const message::Message &message) const {

   Communicator::the().sendUi(message);
   return true;
}

bool ModuleManager::sendMessage(const int moduleId, const message::Message &message) const {

   RunningMap::const_iterator it = runningMap.find(moduleId);
   if (it == runningMap.end()) {
      CERR << "sendMessage: module " << moduleId << " not found" << std::endl;
      return false;
   }


   if (it->second.local)
      it->second.sendQueue->getMessageQueue().send(&message, message.size(), 0);

   return true;
}

bool ModuleManager::handle(const message::Ping &ping) {

   m_stateTracker.handle(ping);
   sendAll(ping);
   return true;
}

bool ModuleManager::handle(const message::Pong &pong) {

   m_stateTracker.handle(pong);
   //CERR << "Pong [" << pong.senderId() << " " << pong.getCharacter() << "]" << std::endl;
   return true;
}

bool ModuleManager::handle(const message::Spawn &spawn) {

   int moduleID = spawn.spawnId();
   if (moduleID == 0) {
      moduleID = newModuleID();
   }
   message::Spawn toUi = spawn;
   toUi.setSpawnId(moduleID);
   sendUi(toUi);
   m_stateTracker.handle(toUi);
   sendAll(toUi);

   int numSpwan = spawn.getMpiSize();
   if (numSpwan <= 0 || numSpwan > m_size)
      numSpwan = m_size;
   int rankSkip = spawn.getRankSkip();
   if (rankSkip < 0)
      rankSkip = 0;
   int baseRank = spawn.getBaseRank();
   if (baseRank < 0)
      baseRank = 0;
   if (baseRank + numSpwan*(rankSkip+1) > m_size)
      numSpwan = (m_size-baseRank+rankSkip)/(rankSkip+1);

   bool onThisRank = true;
   if (m_rank < baseRank)
      onThisRank = false;
   if (m_rank >= baseRank+numSpwan*(rankSkip+1))
      onThisRank = false;
   if ((baseRank + m_rank) % (rankSkip+1) != 0)
      onThisRank = false;

   //CERR << "spawning " << spawn.getName() << ": size=" << m_size << ", num=" << numSpwan << ", base=" << baseRank << ", skip=" << rankSkip << ", local: " << onThisRank << std::endl;

   std::string name = m_bindir + "/" + spawn.getName();

   std::stringstream modID;
   modID << moduleID;
   std::string id = modID.str();

   Module &mod = runningMap[moduleID];
   mod.local = onThisRank;
   mod.baseRank = baseRank;

   if (onThisRank) {
      int localRank = (m_rank-baseRank) / (rankSkip+1);
      std::string smqName = message::MessageQueue::createName("smq", moduleID, localRank);
      std::string rmqName = message::MessageQueue::createName("rmq", moduleID, localRank);

      try {
         mod.sendQueue = message::MessageQueue::create(smqName);
         mod.recvQueue = message::MessageQueue::create(rmqName);
      } catch (bi::interprocess_exception &ex) {

         CERR << "spawn mq " << ex.what() << std::endl;
         exit(-1);
      }
   }

   std::string executable = name;
   std::vector<const char *> argv;
#if 0
   if (spawn.getDebugFlag()) {
      CERR << "spawn with debug on rank " << spawn.getDebugRank() << std::endl;
      executable = "debug_vistle.sh";
      argv.push_back(name.c_str());
   }
#endif

   argv.push_back(Shm::the().name().c_str());
   argv.push_back(id.c_str());
   argv.push_back(NULL);

   std::vector<char *> commands(numSpwan, const_cast<char *>(executable.c_str()));
   std::vector<char **> argvs(numSpwan, const_cast<char **>(&argv[0]));
   std::vector<int> maxprocs(numSpwan, 1);
   std::vector<MPI_Info> infos(numSpwan);
   for (int i=0; i<infos.size(); ++i) {
      MPI_Info_create(&infos[i]);
      int hostidx = baseRank + i*(rankSkip+1);
      MPI_Info_set(infos[i], const_cast<char *>("host"), const_cast<char *>(m_hosts[hostidx].c_str()));
   }
   std::vector<int> errors(infos.size());

   MPI_Comm interComm;
   MPI_Comm_spawn_multiple(infos.size(), &commands[0], &argvs[0], &maxprocs[0], &infos[0],
         0, MPI_COMM_WORLD, &interComm, &errors[0]);

   for (int i=0; i<infos.size(); ++i) {
      MPI_Info_free(&infos[i]);
   }

   // inform newly started module about current parameter values of other modules
   for (auto &mit: runningMap) {
      const int id = mit.first;
      const std::string moduleName = getModuleName(mit.first);

      message::Spawn spawn(id, moduleName);
      spawn.setRank(m_rank);
      sendMessage(moduleID, spawn);

      for (std::string paramname: m_stateTracker.getParameters(id)) {
         const Parameter *param = m_stateTracker.getParameter(id, paramname);

         message::AddParameter add(param, moduleName);
         add.setSenderId(id);
         add.setRank(m_rank);
         sendMessage(moduleID, add);

         message::SetParameter set(id, paramname, param);
         set.setSenderId(id);
         set.setRank(m_rank);
         sendMessage(moduleID, set);
      }
   }

   return true;
}

bool ModuleManager::handle(const message::Started &started) {

   m_stateTracker.handle(started);
   // FIXME: not valid for cover
   //assert(m_stateTracker.getModuleName(moduleID) == started.getName());

   replayMessages();
   return true;
}

bool ModuleManager::handle(const message::Connect &connect) {

   m_stateTracker.handle(connect);
   if (m_portManager.addConnection(connect.getModuleA(),
            connect.getPortAName(),
            connect.getModuleB(),
            connect.getPortBName())) {
      // inform modules about connections
      sendMessage(connect.getModuleA(), connect);
      sendMessage(connect.getModuleB(), connect);
   } else {
      queueMessage(connect);
   }
   return true;
}

bool ModuleManager::handle(const message::Disconnect &disconnect) {

   m_stateTracker.handle(disconnect);
   if (m_portManager.removeConnection(disconnect.getModuleA(),
            disconnect.getPortAName(),
            disconnect.getModuleB(),
            disconnect.getPortBName())) {

      sendMessage(disconnect.getModuleA(), disconnect);
      sendMessage(disconnect.getModuleB(), disconnect);
   } else {

      if (!m_messageQueue.empty()) {
         // only if messages are already queued, there is a chance that this
         // connection might still be established
         queueMessage(disconnect);
      }
   }

   return true;
}

bool ModuleManager::handle(const message::ModuleExit &moduleExit) {

   sendAll(moduleExit);

   int mod = moduleExit.senderId();
   if (!moduleExit.isForwarded()) {

      RunningMap::iterator it = runningMap.find(mod);
      if (it == runningMap.end()) {
         CERR << " Module [" << mod << "] quit, but not found in running map" << std::endl;
         return false;
      }
      Module &m = it->second;
      if (m.baseRank == m_rank) {
         message::ModuleExit exit = moduleExit;
         exit.setForwarded();
         if (!Communicator::the().broadcastAndHandleMessage(exit))
            return false;
      }
      return true;
   }

   m_stateTracker.handle(moduleExit);

   CERR << " Module [" << mod << "] quit" << std::endl;

   { 
      RunningMap::iterator it = runningMap.find(mod);
      if (it != runningMap.end()) {
         Module &mod = it->second;
         if (m_rank == mod.baseRank) {
         }
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
   m_portManager.removeConnections(mod);
   if (m_activeBarrier >= 0 && checkBarrier(m_activeBarrier))
      barrierReached(m_activeBarrier);

   return true;
}

bool ModuleManager::handle(const message::Compute &compute) {

   m_stateTracker.handle(compute);
   RunningMap::iterator i = runningMap.find(compute.getModule());
   if (i != runningMap.end()) {
      if (compute.senderId() == 0) {
         i->second.sendQueue->getMessageQueue().send(&compute, sizeof(compute), 0);
         if (compute.getExecutionCount() > m_executionCounter)
            m_executionCounter = compute.getExecutionCount();
      } else {
         message::Compute msg(compute.getModule(), newExecutionCount());
         msg.setSenderId(compute.senderId());
         msg.setUuid(compute.uuid());
         i->second.sendQueue->getMessageQueue().send(&msg, sizeof(msg), 0);
      }
   }

   return true;
}

bool ModuleManager::handle(const message::Busy &busy) {

   return m_stateTracker.handle(busy);
}

bool ModuleManager::handle(const message::Idle &idle) {

   return m_stateTracker.handle(idle);
}

bool ModuleManager::handle(const message::AddParameter &addParam) {

   m_stateTracker.handle(addParam);
#ifdef DEBUG
   CERR << "AddParameter: module=" << addParam.moduleName() << "(" << addParam.senderId() << "), name=" << addParam.getName() << std::endl;
#endif

   // let all modules know that a parameter was added
   sendAll(addParam);

   replayMessages();
   return true;
}

bool ModuleManager::handle(const message::SetParameter &setParam) {

   m_stateTracker.handle(setParam);
#ifdef DEBUG
   CERR << "SetParameter: sender=" << setParam.senderId() << ", module=" << setParam.getModule() << ", name=" << setParam.getName() << std::endl;
#endif

   Parameter *param = getParameter(setParam.getModule(), setParam.getName());
   Parameter *applied = NULL;
   if (param) {
      applied = param->clone();
      setParam.apply(applied);
   }
   if (setParam.senderId() != setParam.getModule()) {
      // message to owning module
      RunningMap::iterator i = runningMap.find(setParam.getModule());
      if (i != runningMap.end() && param)
         i->second.sendQueue->getMessageQueue().send(&setParam, setParam.size(), 0);
      else
         queueMessage(setParam);
   } else {
      // notification of owning module about a changed parameter
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
      typedef std::set<Parameter *> ParameterSet;

      std::function<ParameterSet (const Port *, ParameterSet)> findAllConnectedPorts;
      findAllConnectedPorts = [this, &findAllConnectedPorts] (const Port *port, ParameterSet conn) -> ParameterSet {
         if (const Port::PortSet *list = this->m_portManager.getConnectionList(port)) {
            for (auto port: *list) {
               Parameter *param = getParameter(port->getModuleID(), port->getName());
               if (param && conn.find(param) == conn.end()) {
                  conn.insert(param);
                  const Port *port = m_portManager.getPort(param->module(), param->getName());
                  conn = findAllConnectedPorts(port, conn);
               }
            }
         }
         return conn;
      };

      Port *port = m_portManager.getPort(setParam.getModule(), setParam.getName());
      if (port && applied) {
         ParameterSet conn = findAllConnectedPorts(port, ParameterSet());

         for (ParameterSet::iterator it = conn.begin();
               it != conn.end();
               ++it) {
            const Parameter *p = *it;
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
   delete applied;

   return true;
}

bool ModuleManager::handle(const message::Kill &kill) {

   m_stateTracker.handle(kill);
   sendMessage(kill.getModule(), kill);
   return true;
}

bool ModuleManager::handle(const message::AddObject &addObj) {

   m_stateTracker.handle(addObj);
   Object::const_ptr obj = addObj.takeObject();
   assert(obj->refcount() >= 1);
#if 0
   std::cerr << "Module " << addObj.senderId() << ": "
      << "AddObject " << addObj.getHandle() << " (" << obj->getName() << ")"
      << " ref " << obj->refcount()
      << " to port " << addObj.getPortName() << std::endl;
#endif

   Port *port = m_portManager.getPort(addObj.senderId(), addObj.getPortName());
   const Port::PortSet *list = NULL;
   if (port) {
      list = m_portManager.getConnectionList(port);
   }
   if (list) {
      Port::PortSet::const_iterator pi;
      for (pi = list->begin(); pi != list->end(); pi ++) {

         int destId = (*pi)->getModuleID();

         message::AddObject a((*pi)->getName(), obj);
         a.setSenderId(addObj.senderId());
         a.setUuid(addObj.uuid());
         a.setRank(addObj.rank());
         sendMessage(destId, a);

         message::Compute c(destId, -1);
         c.setUuid(addObj.uuid());
         sendMessage(destId, c);

         message::ObjectReceived recv(addObj.getPortName(), obj);
         recv.setUuid(addObj.uuid());
         recv.setSenderId(destId);

         if (!Communicator::the().broadcastAndHandleMessage(recv))
            return false;
      }
   }
   else
      CERR << "AddObject ["
         << addObj.getHandle() << "] to port ["
         << addObj.getPortName() << "] of [" << addObj.senderId() << "]: port not found" << std::endl;

   return true;
}

bool ModuleManager::handle(const message::ObjectReceived &objRecv) {

   m_stateTracker.handle(objRecv);
   sendMessage(objRecv.senderId(), objRecv);
   return true;
}

bool ModuleManager::handle(const message::Barrier &barrier) {

   m_stateTracker.handle(barrier);
   CERR << "Barrier [" << barrier.getBarrierId() << "]" << std::endl;
   assert(m_activeBarrier == -1);
   m_activeBarrier = barrier.getBarrierId();
   if (checkBarrier(m_activeBarrier)) {
      barrierReached(m_activeBarrier);
   } else {
      sendAll(barrier);
   }
   return true;
}

bool ModuleManager::handle(const message::BarrierReached &barrReached) {

   m_stateTracker.handle(barrReached);
#ifdef DEBUG
   CERR << "BarrierReached [barrier " << barrReached.getBarrierId() << ", module " << barrReached.senderId() << "]" << std::endl;
#endif
   if (m_activeBarrier == -1) {
      m_activeBarrier = barrReached.getBarrierId();
   }
   assert(m_activeBarrier == barrReached.getBarrierId());
   reachedSet.insert(barrReached.senderId());

   if (checkBarrier(m_activeBarrier)) {
      barrierReached(m_activeBarrier);
#ifdef DEBUG
   } else {
      CERR << "BARRIER: reached by " << reachedSet.size() << "/" << runningMap.size() << std::endl;
#endif
   }

   return true;
}

bool ModuleManager::handle(const message::CreatePort &createPort) {

   m_stateTracker.handle(createPort);
   replayMessages();
   return true;
}

ModuleManager::~ModuleManager() {

   sendAll(message::Quit());

   // receive all ModuleExit messages from modules
   // retry for some time, modules that don't answer might have crashed
   CERR << "waiting for " << runningMap.size() << " modules to quit" << std::endl;
   int retries = 10000;
   while (runningMap.size() > 0 && --retries >= 0) {
      Communicator::the().dispatch();
      usleep(1000);
   }

   if (runningMap.size() > 0)
      CERR << runningMap.size() << " modules failed to quit" << std::endl;

   if (m_size > 0)
      MPI_Barrier(MPI_COMM_WORLD);
}

std::vector<char> ModuleManager::getState() const {

   return m_stateTracker.getState();
}

const PortManager &ModuleManager::portManager() const {

   return m_portManager;
}

std::vector<std::string> ModuleManager::getParameters(int id) const {

   return m_stateTracker.getParameters(id);
}

Parameter *ModuleManager::getParameter(int id, const std::string &name) const {

   return m_stateTracker.getParameter(id, name);
}

bool ModuleManager::checkMessageQueue() const {

   if (!m_messageQueue.empty()) {
      CERR << m_messageQueue.size()/message::Message::MESSAGE_SIZE << " not ready for processing:" << std::endl;
      for (size_t i=0; i<m_messageQueue.size(); i+=message::Message::MESSAGE_SIZE) {
         const message::Message &m = *static_cast<const message::Message *>(static_cast<const void *>(&m_messageQueue[i]));
         std::cerr << "   " << "from: " << m.senderId() << ", type: " << m.type() << std::endl;
      }
   }
   return m_messageQueue.empty();
}

void ModuleManager::queueMessage(const message::Message &msg) {

   const char *m = static_cast<const char *>(static_cast<const void *>(&msg));
   std::copy(m, m+message::Message::MESSAGE_SIZE, std::back_inserter(m_messageQueue));
   //CERR << "queueing " << msg.type() << ", now " << m_messageQueue.size()/message::Message::MESSAGE_SIZE << " in queue" << std::endl;
}

void ModuleManager::replayMessages() {

   std::vector<char> queue;
   std::swap(m_messageQueue, queue);
   //CERR << "replaying " << queue.size()/message::Message::MESSAGE_SIZE << " messages" << std::endl;
   for (size_t i=0; i<queue.size(); i+=message::Message::MESSAGE_SIZE) {
      const message::Message &m = *static_cast<const message::Message *>(static_cast<const void *>(&queue[i]));
      Communicator::the().handleMessage(m);
   }
}

} // namespace vistle
