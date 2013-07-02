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
   std::cerr << "mod mgr [" << rank << "/" << size << "] "

#define DEBUG

using namespace boost::interprocess;

namespace vistle {

ModuleManager::ModuleManager(int argc, char *argv[], int r, const std::vector<std::string> &hosts)
: rank(r)
, size(hosts.size())
, m_hosts(hosts)
, m_portManager(this)
, m_moduleCounter(0)
, m_executionCounter(0)
, m_barrierCounter(0)
, m_activeBarrier(-1)
, m_reachedBarriers(-1)
{
   message::DefaultSender::init(0, rank);

   m_bindir = getbindir(argc, argv);
}

int ModuleManager::getRank() const {

   return rank;
}

int ModuleManager::getSize() const {

   return size;
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

   std::vector<int> result;
   for (RunningMap::const_iterator it = runningMap.begin();
         it != runningMap.end();
         ++it) {
      result.push_back(it->first);
   }
   return result;
}

std::vector<int> ModuleManager::getBusyList() const {

   std::vector<int> result;
   for (ModuleSet::const_iterator it = busySet.begin();
         it != busySet.end();
         ++it) {
      result.push_back(*it);
   }
   return result;
}

std::string ModuleManager::getModuleName(int id) const {

   RunningMap::const_iterator it = runningMap.find(id);
   if (it == runningMap.end())
      return std::string();

   return it->second.name;
}


bool ModuleManager::dispatch(bool &received) {

   bool done = false;

   // handle messages from modules
   for (std::map<int, message::MessageQueue *>::iterator next, i = receiveMessageQueue.begin();
         i != receiveMessageQueue.end();
         i = next) {
      next = i;
      ++next;

      // keep messages from modules that have already reached a barrier on hold
      if (reachedSet.find(i->first) != reachedSet.end())
         continue;

      boost::interprocess::message_queue &mq = i->second->getMessageQueue();

      if (!Communicator::the().tryReceiveAndHandleMessage(mq, received))
         done = true;
   }

   return true;
}

bool ModuleManager::sendAll(const message::Message &message) const {

   for (MessageQueueMap::const_iterator i = sendMessageQueue.begin();
         i != sendMessageQueue.end();
         ++i) {
      i->second->getMessageQueue().send(&message, message.size(), 0);
   }

   return true;
}

bool ModuleManager::sendMessage(const int moduleId, const message::Message &message) const {

   MessageQueueMap::const_iterator it = sendMessageQueue.find(moduleId);
   if (it == sendMessageQueue.end()) {
      CERR << "sendMessage: module " << moduleId << " not found" << std::endl;
      return false;
   }

   it->second->getMessageQueue().send(&message, message.size(), 0);

   return true;
}

bool ModuleManager::handle(const message::Ping &ping) {

   sendAll(ping);
   return true;
}

bool ModuleManager::handle(const message::Pong &pong) {

   CERR << "Pong [" << pong.senderId() << " " << pong.getCharacter() << "]" << std::endl;
   return true;
}

bool ModuleManager::handle(const message::Spawn &spawn) {

   int moduleID = spawn.getSpawnID();

   std::string name = m_bindir + "/" + spawn.getName();

   std::stringstream modID;
   modID << moduleID;
   std::string id = modID.str();

   std::string smqName = message::MessageQueue::createName("smq", moduleID, rank);
   std::string rmqName = message::MessageQueue::createName("rmq", moduleID, rank);

   try {
      sendMessageQueue[moduleID] = message::MessageQueue::create(smqName);
      receiveMessageQueue[moduleID] = message::MessageQueue::create(rmqName);
   } catch (interprocess_exception &ex) {

      CERR << "spawn mq " << ex.what() << std::endl;
      exit(-1);
   }

   std::string executable = name;
   std::vector<const char *> argv;
   if (spawn.getDebugFlag()) {
      CERR << "spawn with debug on rank " << spawn.getDebugRank() << std::endl;
      executable = "debug_vistle.sh";
      argv.push_back(name.c_str());
   }

   MPI_Comm interComm;
   argv.push_back(Shm::the().name().c_str());
   argv.push_back(id.c_str());
   argv.push_back(NULL);

   std::vector<char *> commands(size, const_cast<char *>(executable.c_str()));
   std::vector<char **> argvs(size, const_cast<char **>(&argv[0]));
   std::vector<int> maxprocs(size, 1);
   std::vector<MPI_Info> infos(size);
   for (int i=0; i<infos.size(); ++i) {
      MPI_Info_create(&infos[i]);
      MPI_Info_set(infos[i], const_cast<char *>("host"), const_cast<char *>(m_hosts[i].c_str()));
   }
   std::vector<int> errors(size);

   MPI_Comm_spawn_multiple(size, &commands[0], &argvs[0], &maxprocs[0], &infos[0],
         0, MPI_COMM_WORLD, &interComm, &errors[0]);

   for (int i=0; i<infos.size(); ++i) {
      MPI_Info_free(&infos[i]);
   }

   runningMap[moduleID].name = spawn.getName();

   // inform modules about current parameter values of other modules
   for (auto mit: parameterMap) {
      ModuleParameterMap &pm = mit.second;
      const std::string moduleName = getModuleName(mit.first);
      for (auto pit: pm) {
         message::AddParameter add(pit.first, pit.second->description(),
               pit.second->type(), pit.second->presentation(), moduleName);
         add.setSenderId(mit.first);
         add.setRank(rank);
         sendMessage(moduleID, add);

         message::SetParameter set(mit.first, pit.first, pit.second);
         set.setSenderId(mit.first);
         set.setRank(rank);
         sendMessage(moduleID, set);
      }
   }

   return true;
}

bool ModuleManager::handle(const message::Started &started) {

   int moduleID = started.senderId();
   runningMap[moduleID].initialized = true;
   assert(runningMap[moduleID].name == started.getName());

   replayMessages();
   return true;
}

bool ModuleManager::handle(const message::Connect &connect) {

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

   if (m_portManager.removeConnection(disconnect.getModuleA(),
            disconnect.getPortAName(),
            disconnect.getModuleB(),
            disconnect.getPortBName())) {
      {
         MessageQueueMap::iterator it = sendMessageQueue.find(disconnect.getModuleA());
         if (it != sendMessageQueue.end())
            it->second->getMessageQueue().send(&disconnect, sizeof(disconnect), 0);
      }
      {
         MessageQueueMap::iterator it = sendMessageQueue.find(disconnect.getModuleB());
         if (it != sendMessageQueue.end())
            it->second->getMessageQueue().send(&disconnect, sizeof(disconnect), 0);
      }
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

   int mod = moduleExit.senderId();

   CERR << " Module [" << mod << "] quit" << std::endl;

   {
      ParameterMap::iterator it = parameterMap.find(mod);
      if (it != parameterMap.end()) {
         parameterMap.erase(it);
      }
   }
   { 
      RunningMap::iterator it = runningMap.find(mod);
      if (it != runningMap.end()) {
         runningMap.erase(it);
      } else {
         CERR << " Module [" << mod << "] not found in map" << std::endl;
      }
   }
   {
      MessageQueueMap::iterator i = sendMessageQueue.find(mod);
      if (i != sendMessageQueue.end()) {
         delete i->second;
         sendMessageQueue.erase(i);
      }
   }
   {
      MessageQueueMap::iterator i = receiveMessageQueue.find(mod);
      i = receiveMessageQueue.find(mod);
      if (i != receiveMessageQueue.end()) {
         delete i->second;
         receiveMessageQueue.erase(i);
      }
   }
   {
      ModuleSet::iterator it = busySet.find(mod);
      if (it != busySet.end())
         busySet.erase(it);
   }
   {
      ModuleSet::iterator it = reachedSet.find(mod);
      if (it != reachedSet.end())
         reachedSet.erase(it);
   }
   m_portManager.removeConnections(mod);
   if (m_activeBarrier >= 0 && reachedSet.size() == sendMessageQueue.size())
      barrierReached(m_activeBarrier);

   return true;
}

bool ModuleManager::handle(const message::Compute &compute) {

   MessageQueueMap::iterator i = sendMessageQueue.find(compute.getModule());
   if (i != sendMessageQueue.end()) {
      if (compute.senderId() == 0) {
         i->second->getMessageQueue().send(&compute, sizeof(compute), 0);
         if (compute.getExecutionCount() > m_executionCounter)
            m_executionCounter = compute.getExecutionCount();
      } else {
         message::Compute msg(compute.getModule(), newExecutionCount());
         msg.setSenderId(compute.senderId());
         i->second->getMessageQueue().send(&msg, sizeof(msg), 0);
      }
   }

   return true;
}

bool ModuleManager::handle(const message::Busy &busy) {

   const int id = busy.senderId();
   if (busySet.find(id) != busySet.end()) {
      CERR << "module " << id << " sent Busy twice" << std::endl;
   } else {
      busySet.insert(id);
   }

   return true;
}

bool ModuleManager::handle(const message::Idle &idle) {

   const int id = idle.senderId();
   ModuleSet::iterator it = busySet.find(id);
   if (it != busySet.end()) {
      busySet.erase(it);
   } else {
      CERR << "module " << id << " sent Idle, but was not busy" << std::endl;
   }
   return true;
}

bool ModuleManager::handle(const message::AddParameter &addParam) {

#ifdef DEBUG
   CERR << "AddParameter: module=" << addParam.moduleName() << "(" << addParam.senderId() << "), name=" << addParam.getName() << std::endl;
#endif

   ModuleParameterMap &pm = parameterMap[addParam.senderId()];
   ModuleParameterMap::iterator it = pm.find(addParam.getName());
   if (it != pm.end()) {
      CERR << "double parameter" << std::endl;
   } else {
      pm[addParam.getName()] = addParam.getParameter();
   }

   // let all modules know that a parameter was added
   for (MessageQueueMap::iterator i = sendMessageQueue.begin();
         i != sendMessageQueue.end();
         ++i) {
      i->second->getMessageQueue().send(&addParam, sizeof(addParam), 0);
   }

   m_portManager.addPort(new Port(addParam.senderId(), addParam.getName(), Port::PARAMETER));

   replayMessages();
   return true;
}

bool ModuleManager::handle(const message::SetParameter &setParam) {

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
      MessageQueueMap::iterator i = sendMessageQueue.find(setParam.getModule());
      if (i != sendMessageQueue.end() && param)
         i->second->getMessageQueue().send(&setParam, setParam.size(), 0);
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
      for (MessageQueueMap::iterator i = sendMessageQueue.begin();
            i != sendMessageQueue.end();
            ++i) {
         if (i->first != setParam.senderId())
            i->second->getMessageQueue().send(&setParam, sizeof(setParam), 0);
      }
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
            sendMessage(p->module(), set);
         }
      } else {
         CERR << " SetParameter ["
            << setParam.getModule() << ":" << setParam.getName()
            << "]: port not found" << std::endl;
      }
   }
   delete applied;

   return true;
}

bool ModuleManager::handle(const message::Kill &kill) {

   MessageQueueMap::iterator it = sendMessageQueue.find(kill.getModule());
   if (it != sendMessageQueue.end())
      it->second->getMessageQueue().send(&kill, sizeof(kill), 0);

   return true;
}

bool ModuleManager::handle(const message::AddObject &addObj) {

   Object::const_ptr obj = addObj.takeObject();
   assert(obj->refcount() >= 1);
#if 0
   std::cerr << "Module " << addObj.senderId() << ": "
      << "AddObject " << addObj.getHandle() << " (" << obj->getName() << ")"
      << " ref " << obj->refcount()
      << " to port " << addObj.getPortName() << std::endl;
#endif

   Port *port = m_portManager.getPort(addObj.senderId(),
         addObj.getPortName());
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
         a.setRank(addObj.rank());
         sendMessage(destId, a);

         const message::Compute c(destId, -1);
         sendMessage(destId, c);

         message::ObjectReceived recv(addObj.getPortName(), obj);
         recv.setSenderId(destId);

         if (!Communicator::the().broadcastAndHandleMessage(recv))
            return false;
      }
   }
   else
      std::cerr << "comm [" << rank << "/" << size << "] Addbject ["
         << addObj.getHandle() << "] to port ["
         << addObj.getPortName() << "] of [" << addObj.senderId() << "]: port not found" << std::endl;

   return true;
}

bool ModuleManager::handle(const message::ObjectReceived &objRecv) {

   sendMessage(objRecv.senderId(), objRecv);
   return true;
}

bool ModuleManager::handle(const message::Barrier &barrier) {

   CERR << "Barrier [" << barrier.getBarrierId() << "]" << std::endl;
   assert(m_activeBarrier == -1);
   m_activeBarrier = barrier.getBarrierId();
   if (sendMessageQueue.empty()) {
      barrierReached(m_activeBarrier);
   } else {
      for (MessageQueueMap::iterator i = sendMessageQueue.begin();
            i != sendMessageQueue.end();
            ++i) {
         i->second->getMessageQueue().send(&barrier, sizeof(barrier), 0);
      }
   }
   return true;
}

bool ModuleManager::handle(const message::BarrierReached &barrReached) {

#ifdef DEBUG
   CERR << "BarrierReached [barrier " << barrReached.getBarrierId() << ", module " << barrReached.senderId() << "]" << std::endl;
#endif
   if (m_activeBarrier == -1) {
      m_activeBarrier = barrReached.getBarrierId();
   }
   assert(m_activeBarrier == barrReached.getBarrierId());
   reachedSet.insert(barrReached.senderId());

   if (reachedSet.size() == receiveMessageQueue.size()) {
      barrierReached(m_activeBarrier);
   }

   return true;
}

bool ModuleManager::handle(const message::CreatePort &createPort) {

   m_portManager.addPort(createPort.getPort());
   replayMessages();
   return true;
}

ModuleManager::~ModuleManager() {

   sendAll(message::Quit());

   // receive all ModuleExit messages from modules
   // retry for some time, modules that don't answer might have crashed
   CERR << "waiting for " << sendMessageQueue.size() << " modules to quit" << std::endl;
   int retries = 10000;
   while (sendMessageQueue.size() > 0 && --retries >= 0) {
      Communicator::the().dispatch();
      usleep(1000);
   }

   if (sendMessageQueue.size() > 0)
      CERR << sendMessageQueue.size() << " modules failed to quit" << std::endl;

   if (size > 0)
      MPI_Barrier(MPI_COMM_WORLD);
}

const PortManager &ModuleManager::portManager() const {

   return m_portManager;
}

std::vector<std::string> ModuleManager::getParameters(int id) const {

   std::vector<std::string> result;

   ParameterMap::const_iterator pit = parameterMap.find(id);
   if (pit == parameterMap.end())
      return result;

   const ModuleParameterMap &pmap = pit->second;
   BOOST_FOREACH (ModuleParameterMap::value_type val, pmap) {
      result.push_back(val.first);
   }

   return result;
}

Parameter *ModuleManager::getParameter(int id, const std::string &name) const {

   ParameterMap::const_iterator pit = parameterMap.find(id);
   if (pit == parameterMap.end())
      return NULL;

   ModuleParameterMap::const_iterator mpit = pit->second.find(name);
   if (mpit == pit->second.end())
      return NULL;

   return mpit->second;
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
   CERR << "queueing " << msg.type() << ", now " << m_messageQueue.size()/message::Message::MESSAGE_SIZE << " in queue" << std::endl;
}

void ModuleManager::replayMessages() {

   std::vector<char> queue;
   std::swap(m_messageQueue, queue);
   CERR << "replaying " << queue.size()/message::Message::MESSAGE_SIZE << " messages" << std::endl;
   for (size_t i=0; i<queue.size(); i+=message::Message::MESSAGE_SIZE) {
      const message::Message &m = *static_cast<const message::Message *>(static_cast<const void *>(&queue[i]));
      Communicator::the().handleMessage(m);
   }
}

} // namespace vistle
