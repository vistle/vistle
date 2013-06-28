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

#include "clientmanager.h"
#include "uimanager.h"
#include "communicator.h"

#define CERR \
   std::cerr << "comm [" << rank << "/" << size << "] "

using namespace boost::interprocess;

namespace vistle {

enum MpiTags {
   TagModulue,
   TagToRank0,
   TagToAny,
};

Communicator *Communicator::s_singleton = NULL;

Communicator::Communicator(int argc, char *argv[], int r, const std::vector<std::string> &hosts)
: m_clientManager(NULL)
, m_uiManager(NULL)
, rank(r)
, size(hosts.size())
, m_hosts(hosts)
, m_quitFlag(false)
, mpiReceiveBuffer(NULL)
, mpiMessageSize(0)
, m_moduleCounter(0)
, m_executionCounter(0)
, m_barrierCounter(0)
, m_activeBarrier(-1)
, m_reachedBarriers(-1)
, m_commandQueue(message::MessageQueue::create(message::MessageQueue::createName("command_queue", 0, r)))
{
   assert(s_singleton == NULL);
   s_singleton = this;

   message::DefaultSender::init(0, rank);

   m_bindir = getbindir(argc, argv);

   mpiReceiveBuffer = new char[message::Message::MESSAGE_SIZE];

   // post requests for length of next MPI message
   MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, TagToAny, MPI_COMM_WORLD, &m_reqAny);
   if (rank == 0)
      MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, TagToRank0, MPI_COMM_WORLD, &m_reqToRank0);
}

Communicator &Communicator::the() {

   assert(s_singleton && "make sure to use the vistle Python module only from within vistle");
   if (!s_singleton)
      exit(1);
   return *s_singleton;
}

int Communicator::getRank() const {

   return rank;
}

int Communicator::getSize() const {

   return size;
}

int Communicator::newModuleID() {
   ++m_moduleCounter;

   return m_moduleCounter;
}

int Communicator::newExecutionCount() {
   ++m_executionCounter;

   return m_executionCounter;
}

void Communicator::resetModuleCounter() {

   m_moduleCounter = 0;
}

int Communicator::getBarrierCounter() {

   ++m_barrierCounter;
   return m_barrierCounter;
}

boost::mutex &Communicator::barrierMutex() {

   return m_barrierMutex;
}

boost::condition_variable &Communicator::barrierCondition() {

   return m_barrierCondition;
}

void Communicator::barrierReached(int id) {

   assert(id >= 0);
   MPI_Barrier(MPI_COMM_WORLD);
   m_reachedBarriers = id;
   m_activeBarrier = -1;
   reachedSet.clear();
   CERR << "Barrier [" << id << "] reached" << std::endl;
   m_barrierCondition.notify_all();
}

std::vector<int> Communicator::getRunningList() const {

   std::vector<int> result;
   for (RunningMap::const_iterator it = runningMap.begin();
         it != runningMap.end();
         ++it) {
      result.push_back(it->first);
   }
   return result;
}

std::vector<int> Communicator::getBusyList() const {

   std::vector<int> result;
   for (ModuleSet::const_iterator it = busySet.begin();
         it != busySet.end();
         ++it) {
      result.push_back(*it);
   }
   return result;
}

std::string Communicator::getModuleName(int id) const {

   RunningMap::const_iterator it = runningMap.find(id);
   if (it == runningMap.end())
      return std::string();

   return it->second.name;
}


void Communicator::setInput(const std::string &input) {

   m_initialInput = input;
}

void Communicator::setFile(const std::string &filename) {

   m_initialFile = filename;
}

void Communicator::setQuitFlag() {

   m_quitFlag = true;
}

bool Communicator::dispatch() {

   bool done = false;

   if (rank == 0) {

      if (!m_clientManager) {
         bool file = !m_initialFile.empty();
         m_clientManager = new ClientManager(file ? m_initialFile : m_initialInput,
               file ? ClientManager::File : ClientManager::String);
      }

      done = !m_clientManager->check();

      if (!done)
         done = m_quitFlag;

      if (!m_uiManager) {
         m_uiManager = new UiManager(m_commandQueue);
      }

      if (!done) 
         done = !m_uiManager->check();
   }

   // broadcast messages received from slaves (rank > 0)
   if (rank == 0) {
      int flag;
      MPI_Status status;
      MPI_Test(&m_reqToRank0, &flag, &status);
      if (flag && status.MPI_TAG == TagToRank0) {

         assert(rank == 0);
         MPI_Request r;
         MPI_Irecv(mpiReceiveBuffer, mpiMessageSize, MPI_BYTE, status.MPI_SOURCE, TagToRank0,
               MPI_COMM_WORLD, &r);
         MPI_Wait(&r, MPI_STATUS_IGNORE);
         message::Message *message = (message::Message *) mpiReceiveBuffer;
         if (!broadcastAndHandleMessage(*message))
            done = true;
         MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, TagToRank0, MPI_COMM_WORLD, &m_reqToRank0);
      }
   }

   // test for message size from another MPI node
   //    - receive actual message from broadcast (on any rank)
   //    - receive actual message from slave rank (on rank 0) for broadcasting
   //    - handle message
   //    - post another MPI receive for size of next message
   int flag;
   MPI_Status status;
   MPI_Test(&m_reqAny, &flag, &status);
   if (flag && status.MPI_TAG == TagToAny) {

      MPI_Bcast(mpiReceiveBuffer, mpiMessageSize, MPI_BYTE,
            status.MPI_SOURCE, MPI_COMM_WORLD);

      message::Message *message = (message::Message *) mpiReceiveBuffer;
#if 0
      printf("[%02d] message from [%02d] message type %d size %d\n",
            rank, status.MPI_SOURCE, message->getType(), mpiMessageSize);
#endif
      if (!handleMessage(*message))
         done = true;

      MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, TagToAny, MPI_COMM_WORLD, &m_reqAny);
   }

   // test for messages from modules
   bool received = false;
   do {
      received = false;
      // handle messages from Python front-end
      if (!tryReceiveAndHandleMessage(m_commandQueue->getMessageQueue(), received, true))
         done = true;

      // handle messages from UIs
      for (auto mq: uiInputQueue) {
         if (!tryReceiveAndHandleMessage(mq.second->getMessageQueue(), received, true))
            done = true;
      }

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

         if (!tryReceiveAndHandleMessage(mq, received))
            done = true;

      }
   } while (received);

   return !done;
}

bool Communicator::tryReceiveAndHandleMessage(boost::interprocess::message_queue &mq, bool &received, bool broadcast) {

   bool done = false;
   try {
      size_t msgSize;
      unsigned int priority;
      char msgRecvBuf[vistle::message::Message::MESSAGE_SIZE];

      received = mq.try_receive(
            (void *) msgRecvBuf,
            vistle::message::Message::MESSAGE_SIZE,
            msgSize, priority);

      if (received) {
         message::Message *msg = reinterpret_cast<message::Message *>(msgRecvBuf);
         if (broadcast) {
            if (!broadcastAndHandleMessage(*msg))
               done = true;
         } else {
            if (!handleMessage(*msg))
               done = true;
         }
         return !done;
      }
   } catch (interprocess_exception &ex) {
      CERR << "receive mq " << ex.what() << std::endl;
      exit(-1);
   }

   return !done;
}

bool Communicator::sendMessage(const int moduleId, const message::Message &message) const {

   MessageQueueMap::const_iterator it = sendMessageQueue.find(moduleId);
   if (it == sendMessageQueue.end()) {
      CERR << "sendMessage: module " << moduleId << " not found" << std::endl;
      return false;
   }

   it->second->getMessageQueue().send(&message, message.size(), 0);

   return true;
}

bool Communicator::broadcastAndHandleMessage(const message::Message &message) {

   if (rank == 0) {
      std::vector<MPI_Request> s(size);
      for (int index = 0; index < size; ++index) {
         if (index != rank)
            MPI_Isend(const_cast<unsigned int *>(&message.m_size), 1, MPI_INT, index, TagToAny,
                  MPI_COMM_WORLD, &s[index]);
      }

      MPI_Bcast(const_cast<message::Message *>(&message), message.m_size, MPI_BYTE, rank, MPI_COMM_WORLD);

      const bool result = handleMessage(message);

      // wait for completion
      for (int index=0; index<size; ++index) {

         if (index == rank)
            continue;

         MPI_Wait(&s[index], MPI_STATUS_IGNORE);
      }

      return result;
   } else {
      MPI_Request s1, s2;
      MPI_Isend(const_cast<unsigned int *>(&message.m_size), 1, MPI_INT, 0, TagToRank0, MPI_COMM_WORLD, &s1);
      MPI_Isend(const_cast<message::Message *>(&message), message.m_size, MPI_BYTE, 0, TagToRank0, MPI_COMM_WORLD, &s2);
      MPI_Wait(&s1, MPI_STATUS_IGNORE);
      MPI_Wait(&s2, MPI_STATUS_IGNORE);

      // message will be handled when received again from rank 0
      return true;
   }
}


bool Communicator::handleMessage(const message::Message &message) {

#ifdef DEBUG
   CERR << "Message: "
      << "type=" << message.type() << ", "
      << "size=" << message.size() << ", "
      << "from=" << message.senderId() << ", "
      << "rank=" << message.getRank() << std::endl;
#endif

   switch (message.type()) {

      case message::Message::PING: {

         const message::Ping &ping =
            static_cast<const message::Ping &>(message);
         int count = 0;
         for (MessageQueueMap::iterator i = sendMessageQueue.begin();
               i != sendMessageQueue.end();
               ++i) {
            i->second->getMessageQueue().send(&ping, sizeof(ping), 0);
            ++count;
         }
         CERR << "Ping [" << ping.getCharacter() << "] - " << count << " times" << std::endl;
         break;
      }

      case message::Message::PONG: {

         const message::Pong &pong =
            static_cast<const message::Pong &>(message);
         CERR << "Pong [" << pong.senderId() << " " << pong.getCharacter() << "]" << std::endl;
         break;
      }

      case message::Message::QUIT: {

         const message::Quit &quit =
            static_cast<const message::Quit &>(message);
         m_uiManager->sendMessage(quit);
         return false;
         break;
      }

      case message::Message::SPAWN: {

         const message::Spawn &spawn =
            static_cast<const message::Spawn &>(message);
         m_uiManager->sendMessage(spawn);

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

         break;
      }

      case message::Message::STARTED: {

         const message::Started &started =
            static_cast<const message::Started &>(message);
         m_uiManager->sendMessage(started);

         int moduleID = started.senderId();
         runningMap[moduleID].initialized = true;
         assert(runningMap[moduleID].name == started.getName());

         replayMessages();
         break;
      }

      case message::Message::KILL: {

         const message::Kill &kill = static_cast<const message::Kill &>(message);
         m_uiManager->sendMessage(kill);
         MessageQueueMap::iterator it = sendMessageQueue.find(kill.getModule());
         if (it != sendMessageQueue.end())
            it->second->getMessageQueue().send(&kill, sizeof(kill), 0);
         break;
      }

      case message::Message::CONNECT: {

         const message::Connect &connect =
            static_cast<const message::Connect &>(message);
         m_uiManager->sendMessage(connect);
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
         break;
      }

      case message::Message::DISCONNECT: {

         const message::Disconnect &disc =
            static_cast<const message::Disconnect &>(message);
         m_uiManager->sendMessage(disc);
         if (m_portManager.removeConnection(disc.getModuleA(),
                  disc.getPortAName(),
                  disc.getModuleB(),
                  disc.getPortBName())) {
            {
               MessageQueueMap::iterator it = sendMessageQueue.find(disc.getModuleA());
               if (it != sendMessageQueue.end())
                  it->second->getMessageQueue().send(&disc, sizeof(disc), 0);
            }
            {
               MessageQueueMap::iterator it = sendMessageQueue.find(disc.getModuleB());
               if (it != sendMessageQueue.end())
                  it->second->getMessageQueue().send(&disc, sizeof(disc), 0);
            }
         } else {
            queueMessage(disc);
         }
         break;
      }

      case message::Message::NEWOBJECT: {

         /*
         const message::NewObject *newObject =
            static_cast<const message::NewObject *>(message);
         vistle::Object *object = (vistle::Object *)
            vistle::Shm::the().shm().get_address_from_handle(newObject.getHandle());

         CERR << "NewObject ["
                   << newObject.getHandle() << "] type ["
                   << object.getType() << "] from module ["
                   << newObject.senderId() << "]" << std::endl;
         */
         break;
      }

      case message::Message::MODULEEXIT: {

         const message::ModuleExit &moduleExit =
            static_cast<const message::ModuleExit &>(message);
         m_uiManager->sendMessage(moduleExit);
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
         break;
      }

      case message::Message::COMPUTE: {

         const message::Compute &comp =
            static_cast<const message::Compute &>(message);
         MessageQueueMap::iterator i = sendMessageQueue.find(comp.getModule());
         if (i != sendMessageQueue.end()) {
            if (comp.senderId() == 0) {
               i->second->getMessageQueue().send(&comp, sizeof(comp), 0);
               if (comp.getExecutionCount() > m_executionCounter)
                  m_executionCounter = comp.getExecutionCount();
            } else {
               message::Compute msg(comp.getModule(), newExecutionCount());
               msg.setSenderId(comp.senderId());
               i->second->getMessageQueue().send(&msg, sizeof(msg), 0);
            }
         }
         break;
      }

      case message::Message::BUSY: {

         const message::Busy &busy =
            static_cast<const message::Busy &>(message);
         m_uiManager->sendMessage(busy);

         const int id = busy.senderId();
         if (busySet.find(id) != busySet.end()) {
            CERR << "module " << id << " sent Busy twice" << std::endl;
         } else {
            busySet.insert(id);
         }
         break;
      }

      case message::Message::IDLE: {

         const message::Idle &idle =
            static_cast<const message::Idle &>(message);
         m_uiManager->sendMessage(idle);

         const int id = idle.senderId();
         ModuleSet::iterator it = busySet.find(id);
         if (it != busySet.end()) {
            busySet.erase(it);
         } else {
            CERR << "module " << id << " sent Idle, but was not busy" << std::endl;
         }
         break;
      }

      case message::Message::ADDOBJECT: {

         const message::AddObject &m =
            static_cast<const message::AddObject &>(message);
         Object::const_ptr obj = m.takeObject();
         assert(obj->refcount() >= 1);
#if 0
         std::cerr << "Module " << m.senderId() << ": "
            << "AddObject " << m.getHandle() << " (" << obj->getName() << ")"
            << " ref " << obj->refcount()
            << " to port " << m.getPortName() << std::endl;
#endif

         Port *port = m_portManager.getPort(m.senderId(),
                                          m.getPortName());
         const Port::PortSet *list = NULL;
         if (port) {
            list = m_portManager.getConnectionList(port);
         }
         if (list) {
            Port::PortSet::const_iterator pi;
            for (pi = list->begin(); pi != list->end(); pi ++) {

               int destId = (*pi)->getModuleID();

               message::AddObject a((*pi)->getName(), obj);
               a.setSenderId(m.senderId());
               a.setRank(m.rank());
               sendMessage(destId, a);

               const message::Compute c(destId, -1);
               sendMessage(destId, c);

               message::ObjectReceived recv(m.getPortName(), obj);
               recv.setSenderId(destId);
               if (!broadcastAndHandleMessage(recv))
                  return false;
            }
         }
         else
            std::cerr << "comm [" << rank << "/" << size << "] Addbject ["
                      << m.getHandle() << "] to port ["
                      << m.getPortName() << "] of [" << m.senderId() << "]: port not found" << std::endl;

         break;
      }

      case message::Message::OBJECTRECEIVED: {
         const message::ObjectReceived &m = static_cast<const message::ObjectReceived &>(message);
         sendMessage(m.senderId(), m);
         break;
      }

      case message::Message::SETPARAMETER: {

         const message::SetParameter &m =
            static_cast<const message::SetParameter &>(message);
         m_uiManager->sendMessage(m);

#ifdef DEBUG
         CERR << "SetParameter: sender=" << m.senderId() << ", module=" << m.getModule() << ", name=" << m.getName() << std::endl;
#endif

         Parameter *param = getParameter(m.getModule(), m.getName());
         Parameter *applied = NULL;
         if (param) {
            applied = param->clone();
            m.apply(applied);
         }
         if (m.senderId() != m.getModule()) {
            // message to owning module
            MessageQueueMap::iterator i = sendMessageQueue.find(m.getModule());
            if (i != sendMessageQueue.end() && param)
               i->second->getMessageQueue().send(&m, m.size(), 0);
            else
               queueMessage(m);
         } else {
            // notification of owning module about a changed parameter
            if (param) {
               m.apply(param);
            } else {
               CERR << "no such parameter: module id=" << m.getModule() << ", name=" << m.getName() << std::endl;
            }

            // let all modules know that a parameter was changed
            for (MessageQueueMap::iterator i = sendMessageQueue.begin();
                  i != sendMessageQueue.end();
                  ++i) {
               if (i->first != m.senderId())
                  i->second->getMessageQueue().send(&m, sizeof(m), 0);
            }
         }

         if (!m.isReply()) {

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

            Port *port = m_portManager.getPort(m.getModule(), m.getName());
            if (port && applied) {
               ParameterSet conn = findAllConnectedPorts(port, ParameterSet());

               for (ParameterSet::iterator it = conn.begin();
                     it != conn.end();
                     ++it) {
                  const Parameter *p = *it;
                  if (p->module()==m.getModule() && p->getName()==m.getName()) {
                     // don't update parameter which was set originally again
                     continue;
                  }
                  message::SetParameter set(p->module(), p->getName(), applied);
                  sendMessage(p->module(), set);
               }
            } else {
               CERR << " SetParameter ["
                  << m.getModule() << ":" << m.getName()
                  << "]: port not found" << std::endl;
            }
         }
         delete applied;
         break;
      }

      case message::Message::ADDPARAMETER: {
         
         const message::AddParameter &m =
            static_cast<const message::AddParameter &>(message);
         m_uiManager->sendMessage(m);

#ifdef DEBUG
         CERR << "AddParameter: module=" << m.moduleName() << "(" << m.senderId() << "), name=" << m.getName() << std::endl;
#endif

         ModuleParameterMap &pm = parameterMap[m.senderId()];
         ModuleParameterMap::iterator it = pm.find(m.getName());
         if (it != pm.end()) {
            CERR << "double parameter" << std::endl;
         } else {
            pm[m.getName()] = m.getParameter();
         }

         // let all modules know that a parameter was added
         for (MessageQueueMap::iterator i = sendMessageQueue.begin();
               i != sendMessageQueue.end();
               ++i) {
            i->second->getMessageQueue().send(&m, sizeof(m), 0);
         }

         m_portManager.addPort(new Port(m.senderId(), m.getName(), Port::PARAMETER));

         replayMessages();
         break;
      }

      case message::Message::CREATEPORT: {
         const message::CreatePort &m =
            static_cast<const message::CreatePort &>(message);
         m_uiManager->sendMessage(m);
         m_portManager.addPort(m.getPort());

         replayMessages();
         break;
      }

      case message::Message::BARRIER: {

         const message::Barrier &m =
            static_cast<const message::Barrier &>(message);

         CERR << "Barrier [" << m.getBarrierId() << "]" << std::endl;
         assert(m_activeBarrier == -1);
         m_activeBarrier = m.getBarrierId();
         if (sendMessageQueue.empty()) {
            barrierReached(m_activeBarrier);
         } else {
            for (MessageQueueMap::iterator i = sendMessageQueue.begin();
                  i != sendMessageQueue.end();
                  ++i) {
               i->second->getMessageQueue().send(&m, sizeof(m), 0);
            }
         }
         break;
      }

      case message::Message::BARRIERREACHED: {
         const message::BarrierReached &m =
            static_cast<const message::BarrierReached &>(message);
#ifdef DEBUG
         CERR << "BarrierReached [barrier " << m.getBarrierId() << ", module " << m.senderId() << "]" << std::endl;
#endif
         if (m_activeBarrier == -1) {
            m_activeBarrier = m.getBarrierId();
         }
         assert(m_activeBarrier == m.getBarrierId());
         reachedSet.insert(m.senderId());

         if (reachedSet.size() == receiveMessageQueue.size()) {
            barrierReached(m_activeBarrier);
         }
         break;
      }

      default:

         CERR << "unhandled message from (id "
            << message.senderId() << " rank " << message.rank() << ") "
            << "type " << message.type()
            << std::endl;

         break;

   }

   return true;
}

Communicator::~Communicator() {

   message::Quit quit;

   for (MessageQueueMap::iterator i = sendMessageQueue.begin(); i != sendMessageQueue.end(); ++i)
      i->second->getMessageQueue().send(&quit, sizeof(quit), 1);

   // receive all ModuleExit messages from modules
   // retry for some time, modules that don't answer might have crashed
   CERR << "waiting for " << sendMessageQueue.size() << " modules to quit" << std::endl;
   int retries = 10000;
   while (sendMessageQueue.size() > 0 && --retries >= 0) {
      dispatch();
      usleep(1000);
   }

   delete m_clientManager;
   delete m_uiManager;

   if (size > 1) {
      int dummy = 0;
      MPI_Request s;
      MPI_Isend(&dummy, 1, MPI_INT, (rank + 1) % size, TagToAny, MPI_COMM_WORLD, &s);
      if (rank == 1) {
         MPI_Request s2;
         MPI_Isend(&dummy, 1, MPI_INT, 0, TagToRank0, MPI_COMM_WORLD, &s2);
         //MPI_Wait(&s2, MPI_STATUS_IGNORE);
      }
      //MPI_Wait(&s, MPI_STATUS_IGNORE);
      //MPI_Wait(&m_reqAny, MPI_STATUS_IGNORE);
   }
   MPI_Barrier(MPI_COMM_WORLD);

   delete[] mpiReceiveBuffer;
}

const PortManager &Communicator::portManager() const {

   return m_portManager;
}

std::vector<std::string> Communicator::getParameters(int id) const {

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

Parameter *Communicator::getParameter(int id, const std::string &name) const {

   ParameterMap::const_iterator pit = parameterMap.find(id);
   if (pit == parameterMap.end())
      return NULL;

   ModuleParameterMap::const_iterator mpit = pit->second.find(name);
   if (mpit == pit->second.end())
      return NULL;

   return mpit->second;
}

bool Communicator::checkMessageQueue() {

   if (!m_messageQueue.empty()) {
      CERR << m_messageQueue.size()/message::Message::MESSAGE_SIZE << " not ready for processing:" << std::endl;
      for (size_t i=0; i<m_messageQueue.size(); i+=message::Message::MESSAGE_SIZE) {
         const message::Message &m = *static_cast<const message::Message *>(static_cast<const void *>(&m_messageQueue[i]));
         std::cerr << "   " << "from: " << m.senderId() << ", type: " << m.type() << std::endl;
      }
   }
   return m_messageQueue.empty();
}

void Communicator::queueMessage(const message::Message &msg) {

   const char *m = static_cast<const char *>(static_cast<const void *>(&msg));
   std::copy(m, m+message::Message::MESSAGE_SIZE, std::back_inserter(m_messageQueue));
}

void Communicator::replayMessages() {

   std::vector<char> queue;
   std::swap(m_messageQueue, queue);
   for (size_t i=0; i<queue.size(); i+=message::Message::MESSAGE_SIZE) {
      const message::Message &m = *static_cast<const message::Message *>(static_cast<const void *>(&queue[i]));
      handleMessage(m);
   }
}

} // namespace vistle
