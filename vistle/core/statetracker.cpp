#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#include "message.h"
#include "parameter.h"
#include "port.h"
#include "porttracker.h"

#include "statetracker.h"

#define CERR \
   std::cerr << "state tracker: "

//#define DEBUG

namespace bi = boost::interprocess;

namespace vistle {

int StateTracker::Module::state() const {

   int s = 0;
   if (initialized)
      s |= StateObserver::Initialized;
   if (busy)
      s |= StateObserver::Busy;
   if (killed)
      s |= StateObserver::Killed;
   return s;
}

StateTracker::StateTracker(PortTracker *portTracker)
: m_portTracker(portTracker)
{
   assert(m_portTracker);
}

StateTracker::mutex &StateTracker::getMutex() {

   return m_replyMutex;
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

std::string StateTracker::getModuleName(int id) const {

   RunningMap::const_iterator it = runningMap.find(id);
   if (it == runningMap.end())
      return std::string();

   return it->second.name;
}

int StateTracker::getModuleState(int id) const {

   RunningMap::const_iterator it = runningMap.find(id);
   if (it == runningMap.end())
      return 0;

   return it->second.state();
}

static void appendMessage(std::vector<char> &v, const message::Message &msg) {

   assert(v.size() % message::Message::MESSAGE_SIZE == 0);

   size_t sz = msg.size();
   size_t fill = message::Message::MESSAGE_SIZE - sz;
   std::copy((char *)&msg, ((char *)&msg)+sz, std::back_inserter(v));
   v.resize(v.size()+fill);

   assert(v.size() % message::Message::MESSAGE_SIZE == 0);
}

std::vector<char> StateTracker::getState() const {

   using namespace vistle::message;
   std::vector<char> state;

   // modules with parameters and ports
   for (auto &it: runningMap) {
      const int id = it.first;
      const Module &m = it.second;

      Spawn spawn(id, m.name);
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
         const Parameter *param = it3->second;

         AddParameter add(param, getModuleName(id));
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

      for (auto &portname: portTracker()->getInputPortNames(id)) {
         CreatePort cp(portTracker()->getPort(id, portname));
         cp.setSenderId(id);
         appendMessage(state, cp);
      }

      for (auto &portname: portTracker()->getOutputPortNames(id)) {
         CreatePort cp(portTracker()->getPort(id, portname));
         cp.setSenderId(id);
         appendMessage(state, cp);
      }
   }

   // connections
   for (auto &it: runningMap) {
      const int id = it.first;

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

   return state;
}

const std::vector<StateTracker::AvailableModule> &StateTracker::availableModules() const {

    return m_availableModules;
}

bool StateTracker::handle(const message::Message &msg) {

   mutex_locker locker(getMutex());
   using namespace vistle::message;

   switch (msg.type()) {
      case Message::IDENTIFY: {
         break;
      }
      case Message::SPAWN: {
         const Spawn &spawn = static_cast<const Spawn &>(msg);
         registerReply(msg.uuid(), msg);
         handlePriv(spawn);
         break;
      }
      case Message::EXEC: {
         break;
      }
      case Message::STARTED: {
         const Started &started = static_cast<const Started &>(msg);
         handlePriv(started);
         break;
      }
      case Message::KILL: {
         const Kill &kill = static_cast<const Kill &>(msg);
         handlePriv(kill);
         break;
      }
      case Message::QUIT: {
         const Quit &quit = static_cast<const Quit &>(msg);
         handlePriv(quit);
         break;
      }
      case Message::MODULEEXIT: {
         const ModuleExit &modexit = static_cast<const ModuleExit &>(msg);
         handlePriv(modexit);
         break;
      }
      case Message::COMPUTE: {
         break;
      }
      case Message::CREATEPORT: {
         const CreatePort &cp = static_cast<const CreatePort &>(msg);
         handlePriv(cp);
         break;
      }
      case Message::ADDOBJECT: {
         break;
      }
      case Message::OBJECTRECEIVED: {
         break;
      }
      case Message::CONNECT: {
         const Connect &conn = static_cast<const Connect &>(msg);
         handlePriv(conn);
         break;
      }
      case Message::DISCONNECT: {
         const Disconnect &disc = static_cast<const Disconnect &>(msg);
         handlePriv(disc);
         break;
      }
      case Message::ADDPARAMETER: {
         const AddParameter &add = static_cast<const AddParameter &>(msg);
         handlePriv(add);
         break;
      }
      case Message::SETPARAMETER: {
         const SetParameter &set = static_cast<const SetParameter &>(msg);
         handlePriv(set);
         break;
      }
      case Message::SETPARAMETERCHOICES: {
         const SetParameterChoices &choice = static_cast<const SetParameterChoices &>(msg);
         handlePriv(choice);
         break;
      }
      case Message::PING: {
         const Ping &ping = static_cast<const Ping &>(msg);
         handlePriv(ping);
         break;
      }
      case Message::PONG: {
         const Pong &pong = static_cast<const Pong &>(msg);
         handlePriv(pong);
         break;
      }
      case Message::TRACE: {
         const Trace &trace = static_cast<const Trace &>(msg);
         handlePriv(trace);
         break;
      }
      case Message::BUSY: {
         const Busy &busy = static_cast<const Busy &>(msg);
         handlePriv(busy);
         break;
      }
      case Message::IDLE: {
         const Idle &idle = static_cast<const Idle &>(msg);
         handlePriv(idle);
         break;
      }
      case Message::BARRIER: {
         const Barrier &barrier = static_cast<const Barrier &>(msg);
         handlePriv(barrier);
         break;
      }
      case Message::BARRIERREACHED: {
         const BarrierReached &reached = static_cast<const BarrierReached &>(msg);
         handlePriv(reached);
         registerReply(msg.uuid(), msg);
         break;
      }
      case Message::SETID: {
         const SetId &setid = static_cast<const SetId &>(msg);
         (void)setid;
         break;
      }
      case Message::RESETMODULEIDS: {
         const ResetModuleIds &reset = static_cast<const ResetModuleIds &>(msg);
         handlePriv(reset);
         break;
      }
      case Message::REPLAYFINISHED: {
         const ReplayFinished &fin = static_cast<const ReplayFinished &>(msg);
         handlePriv(fin);
         break;
      }
      case Message::SENDTEXT: {
         const SendText &info = static_cast<const SendText &>(msg);
         handlePriv(info);
         break;
      }
      case Message::MODULEAVAILABLE: {
         const ModuleAvailable &mod = static_cast<const ModuleAvailable &>(msg);
         handlePriv(mod);
         break;
      }
      case Message::EXECUTIONPROGRESS: {
         break;
      }
      case Message::REDUCE: {
         break;
      }
      case Message::LOCKUI: {
         break;
      }
      default:
         CERR << "message type not handled: type=" << msg.type() << std::endl;
         assert("message type not handled" == 0);
         break;
   }

   return true;
}

bool StateTracker::handlePriv(const message::Ping &ping) {

   return true;
}

bool StateTracker::handlePriv(const message::Pong &pong) {

   CERR << "Pong [" << pong.senderId() << " " << pong.getCharacter() << "]" << std::endl;
   return true;
}

bool StateTracker::handlePriv(const message::Trace &trace) {

   return true;
}

bool StateTracker::handlePriv(const message::Spawn &spawn) {

   int moduleId = spawn.spawnId();
   assert(moduleId > 0);

   Module &mod = runningMap[moduleId];
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

   portTracker()->addConnection(connect.getModuleA(),
         connect.getPortAName(),
         connect.getModuleB(),
         connect.getPortBName());

   for (StateObserver *o: m_observers) {
      o->incModificationCount();
      o->newConnection(connect.getModuleA(), connect.getPortAName(),
            connect.getModuleB(), connect.getPortBName());
   }

   return true;
}

bool StateTracker::handlePriv(const message::Disconnect &disconnect) {

   portTracker()->removeConnection(disconnect.getModuleA(),
         disconnect.getPortAName(),
         disconnect.getModuleB(),
         disconnect.getPortBName());

   for (StateObserver *o: m_observers) {
      o->incModificationCount();
      o->deleteConnection(disconnect.getModuleA(), disconnect.getPortAName(),
            disconnect.getModuleB(), disconnect.getPortBName());
   }

   return true;
}

bool StateTracker::handlePriv(const message::ModuleExit &moduleExit) {

   int mod = moduleExit.senderId();

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

bool StateTracker::handlePriv(const message::Compute &compute) {

   return true;
}

bool StateTracker::handlePriv(const message::Reduce &reduce) {

   return true;
}

bool StateTracker::handlePriv(const message::ExecutionProgress &prog) {

   return true;
}

bool StateTracker::handlePriv(const message::Busy &busy) {

   const int id = busy.senderId();
   if (busySet.find(id) != busySet.end()) {
      CERR << "module " << id << " sent Busy twice" << std::endl;
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
      CERR << "module " << id << " sent Idle, but was not busy" << std::endl;
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

   Port *p = portTracker()->addPort(new Port(addParam.senderId(), addParam.getName(), Port::PARAMETER));

   for (StateObserver *o: m_observers) {
      o->newPort(p->getModuleID(), p->getName());
   }

   return true;
}

bool StateTracker::handlePriv(const message::SetParameter &setParam) {

#ifdef DEBUG
   CERR << "SetParameter: sender=" << setParam.senderId() << ", module=" << setParam.getModule() << ", name=" << setParam.getName() << std::endl;
#endif

   Parameter *param = getParameter(setParam.getModule(), setParam.getName());
   if (param)
      setParam.apply(param);

   for (StateObserver *o: m_observers) {
      o->incModificationCount();
      o->parameterValueChanged(setParam.senderId(), setParam.getName());
   }

   return true;
}

bool StateTracker::handlePriv(const message::SetParameterChoices &choices) {

   Parameter *p = getParameter(choices.getModule(), choices.getName());
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

bool StateTracker::handlePriv(const message::CreatePort &createPort) {

   Port * p = portTracker()->addPort(createPort.getPort());

   for (StateObserver *o: m_observers) {
      o->incModificationCount();
      o->newPort(p->getModuleID(), p->getName());
   }

   return true;
}

bool StateTracker::handlePriv(const vistle::message::ResetModuleIds &reset)
{
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
      o->moduleAvailable(mod.name);
   }

    return true;
}



StateTracker::~StateTracker() {

}

PortTracker *StateTracker::portTracker() const {

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

Parameter *StateTracker::getParameter(int id, const std::string &name) const {

   RunningMap::const_iterator rit = runningMap.find(id);
   if (rit == runningMap.end())
      return NULL;

   ParameterMap::const_iterator pit = rit->second.parameters.find(name);
   if (pit == rit->second.parameters.end())
      return NULL;

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

void StateTracker::registerObserver(StateObserver *observer) {

   m_observers.insert(observer);
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
