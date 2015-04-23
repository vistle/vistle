#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#include "message.h"
#include "parameter.h"
#include "port.h"
#include "porttracker.h"

#include "statetracker.h"

#define CERR \
   std::cerr << m_name << ": "

//#define DEBUG

namespace bi = boost::interprocess;

namespace vistle {

namespace {
const std::string unknown("(unknown)");
}

using message::Id;

int StateTracker::Module::state() const {

   int s = StateObserver::Known;
   if (initialized)
      s |= StateObserver::Initialized;
   if (busy)
      s |= StateObserver::Busy;
   if (killed)
      s |= StateObserver::Killed;
   return s;
}

StateTracker::StateTracker(const std::string &name, boost::shared_ptr<PortTracker> portTracker)
: m_portTracker(portTracker)
, m_traceType(message::Message::INVALID)
, m_traceId(Id::Invalid)
, m_name(name)
{
   if (!m_portTracker) {
      m_portTracker.reset(new PortTracker());
      m_portTracker->setTracker(this);
   }
}

StateTracker::mutex &StateTracker::getMutex() {

   return m_replyMutex;
}

int StateTracker::getMasterHub() const {

   return Id::MasterHub;
}

std::vector<int> StateTracker::getHubs() const {
    std::vector<int> hubs;
    for (const auto &h: m_hubs)
       hubs.push_back(h.id);
    return hubs;
}

const std::string &StateTracker::hubName(int id) const {

    for (const auto &h: m_hubs) {
       if (h.id == id)
          return h.name;
    }
    return unknown;
}

std::vector<int> StateTracker::getRunningList() const {

   std::vector<int> result;
   for (RunningMap::const_iterator it = runningMap.begin();
         it != runningMap.end();
         ++it) {
      result.push_back(it->first);
   }
   return result;
}

std::vector<int> StateTracker::getBusyList() const {

   std::vector<int> result;
   for (ModuleSet::const_iterator it = busySet.begin();
         it != busySet.end();
         ++it) {
      result.push_back(*it);
   }
   return result;
}

int StateTracker::getHub(int id) const {

   RunningMap::const_iterator it = runningMap.find(id);
   if (it == runningMap.end())
      return Id::Invalid;

   if (it->second.hub > Id::MasterHub) {
      CERR << "getHub for " << id << " failed" << std::endl;
   }
   assert(it->second.hub <= Id::MasterHub);
   return it->second.hub;
}

std::string StateTracker::getModuleName(int id) const {

   RunningMap::const_iterator it = runningMap.find(id);
   if (it == runningMap.end())
      return std::string();

   return it->second.name;
}

int StateTracker::getModuleState(int id) const {

   RunningMap::const_iterator it = runningMap.find(id);
   if (it == runningMap.end())
      return StateObserver::Unknown;

   return it->second.state();
}

static void appendMessage(std::vector<message::Buffer> &v, const message::Message &msg) {

   v.emplace_back(msg);
}

std::vector<message::Buffer> StateTracker::getState() const {

   using namespace vistle::message;
   std::vector<message::Buffer> state;

   for (const auto &slave: m_hubs) {
      appendMessage(state, AddSlave(slave.id, slave.name));
   }

   // available modules
   auto avail = availableModules();
   for(const auto &mod: avail) {
      appendMessage(state, ModuleAvailable(mod.hub, mod.name, mod.path));
   }

   // modules with parameters and ports
   for (auto &it: runningMap) {
      const int id = it.first;
      const Module &m = it.second;

      Spawn spawn(m.hub, m.name);
      spawn.setSpawnId(id);
      appendMessage(state, spawn);

      if (m.initialized) {
         Started s(m.name);
         s.setSenderId(id);
         s.setUuid(spawn.uuid());
         appendMessage(state, s);
      }

      if (m.busy) {
         Busy b;
         b.setSenderId(id);
         appendMessage(state, b);
      }

      if (m.killed) {
         Kill k(id);
         appendMessage(state, k);
      }

      const ParameterMap &pmap = m.parameters;
      for (const auto &it2: m.paramOrder) {
         //std::cerr << "module " << id << ": " << it2.first << " -> " << it2.second << std::endl;
         const std::string &name = it2.second;
         const auto &it3 = pmap.find(name);
         assert(it3 != pmap.end());
         const auto param = it3->second;

         AddParameter add(*param, getModuleName(id));
         add.setSenderId(id);
         appendMessage(state, add);

         if (param->presentation() == Parameter::Choice) {
            SetParameterChoices choices(id, name, param->choices());
            choices.setSenderId(id);
            appendMessage(state, choices);
         }

		 SetParameter setV(id, name, param, Parameter::Value);
		 setV.setSenderId(id);
		 appendMessage(state, setV);
		 SetParameter setMin(id, name, param, Parameter::Minimum);
		 setMin.setSenderId(id);
		 appendMessage(state, setMin);
		 SetParameter setMax(id, name, param, Parameter::Maximum);
		 setMax.setSenderId(id);
		 appendMessage(state, setMax);
      }

      if (portTracker()) {
         for (auto &portname: portTracker()->getInputPortNames(id)) {
            AddPort cp(portTracker()->getPort(id, portname));
            cp.setSenderId(id);
            appendMessage(state, cp);
         }

         for (auto &portname: portTracker()->getOutputPortNames(id)) {
            AddPort cp(portTracker()->getPort(id, portname));
            cp.setSenderId(id);
            appendMessage(state, cp);
         }
      }
   }

   // connections
   for (auto &it: runningMap) {
      const int id = it.first;

      if (portTracker()) {
         for (auto &portname: portTracker()->getOutputPortNames(id)) {
            const Port::PortSet *connected = portTracker()->getConnectionList(id, portname);
            for (auto &dest: *connected) {
               Connect c(id, portname, dest->getModuleID(), dest->getName());
               appendMessage(state, c);
            }
         }

         for (auto &paramname: getParameters(id)) {
            const Port::PortSet *connected = portTracker()->getConnectionList(id, paramname);
            for (auto &dest: *connected) {
               Connect c(id, paramname, dest->getModuleID(), dest->getName());
               appendMessage(state, c);
            }
         }
      }
   }

   // finalize
   appendMessage(state, ReplayFinished());

   return state;
}

const std::vector<AvailableModule> &StateTracker::availableModules() const {

    return m_availableModules;
}

bool StateTracker::handle(const message::Message &msg, bool track) {

   using namespace vistle::message;

   if (m_traceId != Id::Invalid && m_traceType != Message::INVALID) {

      if (msg.type() == m_traceType || m_traceType == Message::ANY) {

         if (msg.senderId() == m_traceId || msg.destId() == m_traceId || m_traceId == Id::Broadcast) {
            std::cout << m_name << ": " << msg << std::endl << std::flush;
         }
      }
   }

   if (!track)
      return true;

   bool handled = true;

   mutex_locker locker(getMutex());
   switch (msg.type()) {
      case Message::IDENTIFY: {
         break;
      }
      case Message::ADDSLAVE: {
         const AddSlave &slave = static_cast<const AddSlave &>(msg);
         handled = handlePriv(slave);
         break;
      }
      case Message::SPAWN: {
         const Spawn &spawn = static_cast<const Spawn &>(msg);
         registerReply(msg.uuid(), msg);
         handled = handlePriv(spawn);
         break;
      }
      case Message::STARTED: {
         const Started &started = static_cast<const Started &>(msg);
         handled = handlePriv(started);
         break;
      }
      case Message::KILL: {
         const Kill &kill = static_cast<const Kill &>(msg);
         handled = handlePriv(kill);
         break;
      }
      case Message::QUIT: {
         const Quit &quit = static_cast<const Quit &>(msg);
         handled = handlePriv(quit);
         break;
      }
      case Message::MODULEEXIT: {
         const ModuleExit &modexit = static_cast<const ModuleExit &>(msg);
         handled = handlePriv(modexit);
         break;
      }
      case Message::EXECUTE: {
         break;
      }
      case Message::ADDOBJECT: {
         break;
      }
      case Message::OBJECTRECEIVED: {
         break;
      }
      case Message::ADDPORT: {
         const AddPort &cp = static_cast<const AddPort &>(msg);
         handled = handlePriv(cp);
         break;
      }
      case Message::ADDPARAMETER: {
         const AddParameter &add = static_cast<const AddParameter &>(msg);
         handled = handlePriv(add);
         break;
      }
      case Message::CONNECT: {
         const Connect &conn = static_cast<const Connect &>(msg);
         handled = handlePriv(conn);
         break;
      }
      case Message::DISCONNECT: {
         const Disconnect &disc = static_cast<const Disconnect &>(msg);
         handled = handlePriv(disc);
         break;
      }
      case Message::SETPARAMETER: {
         const SetParameter &set = static_cast<const SetParameter &>(msg);
         handled = handlePriv(set);
         break;
      }
      case Message::SETPARAMETERCHOICES: {
         const SetParameterChoices &choice = static_cast<const SetParameterChoices &>(msg);
         handled = handlePriv(choice);
         break;
      }
      case Message::PING: {
         const Ping &ping = static_cast<const Ping &>(msg);
         handled = handlePriv(ping);
         break;
      }
      case Message::PONG: {
         const Pong &pong = static_cast<const Pong &>(msg);
         handled = handlePriv(pong);
         break;
      }
      case Message::TRACE: {
         const Trace &trace = static_cast<const Trace &>(msg);
         handled = handlePriv(trace);
         break;
      }
      case Message::BUSY: {
         const Busy &busy = static_cast<const Busy &>(msg);
         handled = handlePriv(busy);
         break;
      }
      case Message::IDLE: {
         const Idle &idle = static_cast<const Idle &>(msg);
         handled = handlePriv(idle);
         break;
      }
      case Message::BARRIER: {
         const Barrier &barrier = static_cast<const Barrier &>(msg);
         handled = handlePriv(barrier);
         break;
      }
      case Message::BARRIERREACHED: {
         const BarrierReached &reached = static_cast<const BarrierReached &>(msg);
         handled = handlePriv(reached);
         registerReply(msg.uuid(), msg);
         break;
      }
      case Message::SETID: {
         const SetId &setid = static_cast<const SetId &>(msg);
         (void)setid;
         break;
      }
      case Message::REPLAYFINISHED: {
         const ReplayFinished &fin = static_cast<const ReplayFinished &>(msg);
         handled = handlePriv(fin);
         break;
      }
      case Message::SENDTEXT: {
         const SendText &info = static_cast<const SendText &>(msg);
         handled = handlePriv(info);
         break;
      }
      case Message::MODULEAVAILABLE: {
         const ModuleAvailable &mod = static_cast<const ModuleAvailable &>(msg);
         handled = handlePriv(mod);
         break;
      }
      case Message::EXECUTIONPROGRESS: {
         break;
      }
      case Message::LOCKUI: {
         break;
      }
      case Message::OBJECTRECEIVEPOLICY: {
         const ObjectReceivePolicy &m = static_cast<const ObjectReceivePolicy &>(msg);
         handled = handlePriv(m);
         break;
      }
      case Message::SCHEDULINGPOLICY: {
         const SchedulingPolicy &m = static_cast<const SchedulingPolicy &>(msg);
         handled = handlePriv(m);
         break;
      }
      case Message::REDUCEPOLICY: {
         const ReducePolicy &m = static_cast<const ReducePolicy &>(msg);
         handled = handlePriv(m);
         break;
      }
      case Message::REQUESTTUNNEL: {
         const RequestTunnel &m = static_cast<const RequestTunnel &>(msg);
         handled = handlePriv(m);
         break;
      }
      

      default:
         CERR << "message type not handled: " << msg << std::endl;
         assert("message type not handled" == 0);
         break;
   }

   if (handled) {
      if (msg.typeFlags() & TriggerQueue) {
         processQueue();
      }
   } else {
      if (msg.typeFlags() & QueueIfUnhandled) {
         m_queue.emplace_back(msg);
      }
   }

   return true;
}

void StateTracker::processQueue() {

   static bool processing = false;
   if (processing)
      return;
   processing = true;

   std::vector<message::Buffer> queue;
   std::swap(m_queue, queue);

   for (auto &m: queue) {
      handle(m.msg);
   }

   processing = false;
}

bool StateTracker::handlePriv(const message::AddSlave &slave) {
   boost::lock_guard<mutex> locker(m_slaveMutex);
   m_hubs.emplace_back(slave.id(), slave.name());
   m_slaveCondition.notify_all();
   return true;
}

bool StateTracker::handlePriv(const message::Ping &ping) {

   //CERR << "Ping [" << ping.senderId() << " " << ping.getCharacter() << "]" << std::endl;
   return true;
}

bool StateTracker::handlePriv(const message::Pong &pong) {

   CERR << "Pong [" << pong.senderId() << " " << pong.getCharacter() << "]" << std::endl;
   return true;
}

bool StateTracker::handlePriv(const message::Trace &trace) {

   if (trace.on()) {
      m_traceType = trace.messageType();
      m_traceId = trace.module();
      CERR << "tracing " << m_traceType << " from/to " << m_traceId << std::endl;
   } else {
      CERR << "disabling tracing of " << m_traceType << " from/to " << m_traceId << std::endl;
      m_traceId = Id::Invalid;
      m_traceType = message::Message::INVALID;
   }
   return true;
}

bool StateTracker::handlePriv(const message::Spawn &spawn) {

   int moduleId = spawn.spawnId();
   if (moduleId == Id::Invalid) {
      // don't track when master hub has not yet provided a module id
      return true;
   }

   int hub = spawn.hubId();

   Module &mod = runningMap[moduleId];
   mod.hub = hub;
   mod.name = spawn.getName();

   for (StateObserver *o: m_observers) {
      o->incModificationCount();
      o->newModule(moduleId, spawn.uuid(), mod.name);
   }

   return true;
}

bool StateTracker::handlePriv(const message::Started &started) {

   int moduleId = started.senderId();
   runningMap[moduleId].initialized = true;

   for (StateObserver *o: m_observers) {
      o->moduleStateChanged(moduleId, runningMap[moduleId].state());
   }

   return true;
}

bool StateTracker::handlePriv(const message::Connect &connect) {

   if (portTracker()) {
      return portTracker()->addConnection(connect.getModuleA(),
            connect.getPortAName(),
            connect.getModuleB(),
            connect.getPortBName());
   }

   return true;
}

bool StateTracker::handlePriv(const message::Disconnect &disconnect) {

   if (portTracker()) {
      portTracker()->removeConnection(disconnect.getModuleA(),
            disconnect.getPortAName(),
            disconnect.getModuleB(),
            disconnect.getPortBName());
   }

   return true;
}

bool StateTracker::handlePriv(const message::ModuleExit &moduleExit) {

   const int mod = moduleExit.senderId();
   portTracker()->removeConnectionsWithModule(mod);

   CERR << " Module [" << mod << "] quit" << std::endl;

   { 
      RunningMap::iterator it = runningMap.find(mod);
      if (it != runningMap.end()) {
         runningMap.erase(it);
      } else {
         CERR << " ModuleExit [" << mod << "] not found in map" << std::endl;
      }
   }
   {
      ModuleSet::iterator it = busySet.find(mod);
      if (it != busySet.end())
         busySet.erase(it);
   }

   for (StateObserver *o: m_observers) {
      o->incModificationCount();
      o->deleteModule(mod);
   }

   return true;
}

bool StateTracker::handlePriv(const message::Execute &execute) {

   return true;
}

bool StateTracker::handlePriv(const message::ExecutionProgress &prog) {

   return true;
}

bool StateTracker::handlePriv(const message::Busy &busy) {

   const int id = busy.senderId();
   if (busySet.find(id) != busySet.end()) {
      //CERR << "module " << id << " sent Busy twice" << std::endl;
   } else {
      busySet.insert(id);
   }
   runningMap[id].busy = true;

   for (StateObserver *o: m_observers) {
      o->moduleStateChanged(id, runningMap[id].state());
   }

   return true;
}

bool StateTracker::handlePriv(const message::Idle &idle) {

   const int id = idle.senderId();
   ModuleSet::iterator it = busySet.find(id);
   if (it != busySet.end()) {
      busySet.erase(it);
   } else {
      //CERR << "module " << id << " sent Idle, but was not busy" << std::endl;
   }
   runningMap[id].busy = false;

   for (StateObserver *o: m_observers) {
      o->moduleStateChanged(id, runningMap[id].state());
   }

   return true;
}

bool StateTracker::handlePriv(const message::AddParameter &addParam) {

#ifdef DEBUG
   CERR << "AddParameter: module=" << addParam.moduleName() << "(" << addParam.senderId() << "), name=" << addParam.getName() << std::endl;
#endif

   ParameterMap &pm = runningMap[addParam.senderId()].parameters;
   ParameterOrder &po = runningMap[addParam.senderId()].paramOrder;
   ParameterMap::iterator it = pm.find(addParam.getName());
   if (it != pm.end()) {
      CERR << "duplicate parameter " << addParam.moduleName() << ":" << addParam.getName() << std::endl;
   } else {
      pm[addParam.getName()] = addParam.getParameter();
      int maxIdx = 0;
      const auto &rit = po.rbegin();
      if (rit != po.rend())
         maxIdx = rit->first;
      po[maxIdx+1] = addParam.getName();
   }

   for (StateObserver *o: m_observers) {
      o->incModificationCount();
      o->newParameter(addParam.senderId(), addParam.getName());
   }

   if (portTracker()) {
      Port *p = portTracker()->addPort(new Port(addParam.senderId(), addParam.getName(), Port::PARAMETER));


      for (StateObserver *o: m_observers) {
         o->newPort(p->getModuleID(), p->getName());
      }
   }

   return true;
}

bool StateTracker::handlePriv(const message::SetParameter &setParam) {

#ifdef DEBUG
   CERR << "SetParameter: sender=" << setParam.senderId() << ", module=" << setParam.getModule() << ", name=" << setParam.getName() << std::endl;
#endif

   bool handled = false;

   auto param = getParameter(setParam.getModule(), setParam.getName());
   if (param) {
      setParam.apply(param);
      handled = true;
   }

   if (handled) {
      for (StateObserver *o: m_observers) {
         o->incModificationCount();
         o->parameterValueChanged(setParam.senderId(), setParam.getName());
      }
   }

   return handled;
}

bool StateTracker::handlePriv(const message::SetParameterChoices &choices) {

   auto p = getParameter(choices.getModule(), choices.getName());
   if (!p)
      return false;

   choices.apply(p);

   //CERR << "choices changed for " << choices.getModule() << ":" << choices.getName() << ": #" << p->choices().size() << std::endl;

   for (StateObserver *o: m_observers) {
      o->incModificationCount();
      o->parameterChoicesChanged(choices.getModule(), choices.getName());
   }

   return true;
}

bool StateTracker::handlePriv(const message::Quit &quit) {

   for (StateObserver *o: m_observers) {
      o->quitRequested();
   }

   return true;
}

bool StateTracker::handlePriv(const message::Kill &kill) {

   const int id = kill.getModule();
   runningMap[id].killed = true;

   for (StateObserver *o: m_observers) {
      o->moduleStateChanged(id, runningMap[id].state());
   }

   return true;
}

bool StateTracker::handlePriv(const message::AddObject &addObj) {

   return true;
}

bool StateTracker::handlePriv(const message::ObjectReceived &objRecv) {

   return true;
}

bool StateTracker::handlePriv(const message::Barrier &barrier) {

   return true;
}

bool StateTracker::handlePriv(const message::BarrierReached &barrReached) {

   return true;
}

bool StateTracker::handlePriv(const message::AddPort &createPort) {

   if (portTracker()) {
      Port * p = portTracker()->addPort(createPort.getPort());

      for (StateObserver *o: m_observers) {
         o->incModificationCount();
         o->newPort(p->getModuleID(), p->getName());
      }
   }

   return true;
}

bool StateTracker::handlePriv(const message::ReplayFinished &reset)
{
   for (StateObserver *o: m_observers) {
      o->resetModificationCount();
   }
   return true;
}

bool StateTracker::handlePriv(const message::SendText &info)
{
   for (StateObserver *o: m_observers) {
      o->info(info.text(), info.textType(), info.senderId(), info.rank(), info.referenceType(), info.referenceUuid());
   }
   return true;
}

bool StateTracker::handlePriv(const message::ModuleAvailable &avail) {

    AvailableModule mod;
    mod.hub = avail.hub();
    mod.name = avail.name();
    mod.path = avail.path();

    m_availableModules.push_back(mod);

   for (StateObserver *o: m_observers) {
      o->moduleAvailable(mod.hub, mod.name, mod.path);
   }

    return true;
}

bool StateTracker::handlePriv(const message::ObjectReceivePolicy &receivePolicy)
{
   const int id = receivePolicy.senderId();
   RunningMap::iterator it = runningMap.find(id);
   if (it == runningMap.end()) {
      CERR << " Module [" << id << "] changed ObjectReceivePolicy, but not found in running map" << std::endl;
      return false;
   }
   Module &mod = it->second;
   mod.objectPolicy = receivePolicy.policy();
   return true;
}

bool StateTracker::handlePriv(const message::SchedulingPolicy &schedulingPolicy)
{
   const int id = schedulingPolicy.senderId();
   RunningMap::iterator it = runningMap.find(id);
   if (it == runningMap.end()) {
      CERR << " Module [" << id << "] changed SchedulingPolicy, but not found in running map" << std::endl;
      return false;
   }
   Module &mod = it->second;
   mod.schedulingPolicy = schedulingPolicy.policy();
   return true;
}

bool StateTracker::handlePriv(const message::ReducePolicy &reducePolicy)
{
   const int id = reducePolicy.senderId();
   RunningMap::iterator it = runningMap.find(id);
   if (it == runningMap.end()) {
      CERR << " Module [" << id << "] changed ReducePolicy, but not found in running map" << std::endl;
      return false;
   }
   Module &mod = it->second;
   mod.reducePolicy = reducePolicy.policy();
   return true;
}

bool StateTracker::handlePriv(const message::RequestTunnel &tunnel)
{
   return true;
}



StateTracker::~StateTracker() {

    if (m_portTracker) {
        m_portTracker->setTracker(nullptr);
    }
}

boost::shared_ptr<PortTracker> StateTracker::portTracker() const {

   return m_portTracker;
}

std::vector<std::string> StateTracker::getParameters(int id) const {

   std::vector<std::string> result;

   RunningMap::const_iterator rit = runningMap.find(id);
   if (rit == runningMap.end())
      return result;

   const ParameterOrder &po = rit->second.paramOrder;
   BOOST_FOREACH (ParameterOrder::value_type val, po) {
      const auto &name = val.second;
      result.push_back(name);
   }

   return result;
}

boost::shared_ptr<Parameter> StateTracker::getParameter(int id, const std::string &name) const {

   RunningMap::const_iterator rit = runningMap.find(id);
   if (rit == runningMap.end())
      return boost::shared_ptr<Parameter>();

   ParameterMap::const_iterator pit = rit->second.parameters.find(name);
   if (pit == rit->second.parameters.end())
      return boost::shared_ptr<Parameter>();

   return pit->second;
}

bool StateTracker::registerRequest(const message::uuid_t &uuid) {

   boost::lock_guard<mutex> locker(m_replyMutex);

   auto it = m_outstandingReplies.find(uuid);
   if (it != m_outstandingReplies.end()) {
      CERR << "duplicate attempt to wait for reply" << std::endl;
      return false;
   }

   //CERR << "waiting for " << uuid  << std::endl;
   m_outstandingReplies[uuid] = boost::shared_ptr<message::Buffer>();
   return true;
}

boost::shared_ptr<message::Buffer> StateTracker::waitForReply(const message::uuid_t &uuid) {

   boost::unique_lock<mutex> locker(m_replyMutex);
   boost::shared_ptr<message::Buffer> ret = removeRequest(uuid);
   while (!ret) {
      m_replyCondition.wait(locker);
      ret = removeRequest(uuid);
   }
   return ret;
}

boost::shared_ptr<message::Buffer> StateTracker::removeRequest(const message::uuid_t &uuid) {

   //CERR << "remove request try: " << uuid << std::endl;
   boost::shared_ptr<message::Buffer> ret;
   auto it = m_outstandingReplies.find(uuid);
   if (it != m_outstandingReplies.end() && it->second) {
      ret = it->second;
      //CERR << "remove request success: " << uuid << std::endl;
      m_outstandingReplies.erase(it);
   }
   return ret;
}

bool StateTracker::registerReply(const message::uuid_t &uuid, const message::Message &msg) {

   boost::lock_guard<mutex> locker(m_replyMutex);
   auto it = m_outstandingReplies.find(uuid);
   if (it == m_outstandingReplies.end()) {
      return false;
   }
   if (it->second) {
      CERR << "attempt to register duplicate reply for " << uuid << std::endl;
      assert(!it->second);
      return false;
   }

   it->second.reset(new message::Buffer(msg));

   //CERR << "notifying all for " << uuid  << " and " << m_outstandingReplies.size() << " others" << std::endl;

   m_replyCondition.notify_all();

   return true;
}

std::vector<int> StateTracker::waitForSlaveHubs(size_t count) {

   auto hubIds = getHubs();
   while (hubIds.size() < count) {
      boost::unique_lock<mutex> locker(m_slaveMutex);
      m_slaveCondition.wait(locker);
      hubIds = getHubs();
   }
   return hubIds;
}

std::vector<int> StateTracker::waitForSlaveHubs(const std::vector<std::string> &names) {

   auto findAll = [this](const std::vector<std::string> &names, std::vector<int> &ids) -> bool {
      const auto hubIds = getHubs();
      std::vector<std::string> available;
      for (int id: hubIds)
         available.push_back(hubName(id));
      
      ids.clear();
      size_t found=0;
      for (const auto &name: names) {
         for (const auto &slave: m_hubs) {
            if (name == slave.name) {
               ++found;
               ids.push_back(slave.id);
            } else {
               ids.push_back(Id::Invalid);
            }
         }
      }
      return found == names.size();
   };

   std::vector<int> ids;
   while (!findAll(names, ids)) {
      boost::unique_lock<mutex> locker(m_slaveMutex);
      m_slaveCondition.wait(locker);
   }
   return ids;
}

void StateTracker::registerObserver(StateObserver *observer) {

   m_observers.insert(observer);
}

ParameterSet StateTracker::getConnectedParameters(const Parameter &param) const {

   std::function<ParameterSet (const Port *, ParameterSet)> findAllConnectedPorts;
   findAllConnectedPorts = [this, &findAllConnectedPorts] (const Port *port, ParameterSet conn) -> ParameterSet {
      if (const Port::PortSet *list = portTracker()->getConnectionList(port)) {
         for (auto port: *list) {
            auto param = getParameter(port->getModuleID(), port->getName());
            if (param && conn.find(param) == conn.end()) {
               conn.insert(param);
               const Port *port = portTracker()->getPort(param->module(), param->getName());
               conn = findAllConnectedPorts(port, conn);
            }
         }
      }
      return conn;
   };

   if (!portTracker())
      return ParameterSet();
   Port *port = portTracker()->getPort(param.module(), param.getName());
   if (!port)
      return ParameterSet();
   if (port->getType() != Port::PARAMETER)
      return ParameterSet();
   return findAllConnectedPorts(port, ParameterSet());
}

void StateObserver::quitRequested() {

}

void StateObserver::incModificationCount()
{
   ++m_modificationCount;
}

long StateObserver::modificationCount() const
{
   return m_modificationCount;
}

void StateObserver::resetModificationCount()
{
   m_modificationCount = 0;
}

} // namespace vistle
