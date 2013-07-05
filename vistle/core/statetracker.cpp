#include <boost/foreach.hpp>

#include "message.h"
#include "parameter.h"
#include "port.h"
#include "porttracker.h"

#include "statetracker.h"

#define CERR \
   std::cerr << "state tracker "

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

      for (auto &it2: m.parameters) {
         const std::string &name = it2.first;
         const Parameter *param = it2.second;

         AddParameter add(param, getModuleName(id));
         add.setSenderId(id);
         appendMessage(state, add);

         SetParameter set(id, name, param);
         set.setSenderId(id);
         appendMessage(state, set);
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
   }

   return state;
}

bool StateTracker::handleMessage(const message::Message &msg) {

   using namespace vistle::message;

   switch (msg.type()) {
      case Message::DEBUG: {
         break;
      }
      case Message::SPAWN: {
         const Spawn &spawn = static_cast<const Spawn &>(msg);
         return handle(spawn);
         break;
      }
      case Message::STARTED: {
         const Started &started = static_cast<const Started &>(msg);
         return handle(started);
         break;
      }
      case Message::KILL: {
         const Kill &kill = static_cast<const Kill &>(msg);
         return handle(kill);
         break;
      }
      case Message::QUIT: {
         break;
      }
      case Message::NEWOBJECT: {
         break;
      }
      case Message::MODULEEXIT: {
         const ModuleExit &modexit = static_cast<const ModuleExit &>(msg);
         return handle(modexit);
         break;
      }
      case Message::COMPUTE: {
         break;
      }
      case Message::CREATEPORT: {
         const CreatePort &cp = static_cast<const CreatePort &>(msg);
         return handle(cp);
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
         return handle(conn);
         break;
      }
      case Message::DISCONNECT: {
         const Disconnect &disc = static_cast<const Disconnect &>(msg);
         return handle(disc);
         break;
      }
      case Message::ADDPARAMETER: {
         const AddParameter &add = static_cast<const AddParameter &>(msg);
         return handle(add);
         break;
      }
      case Message::SETPARAMETER: {
         const SetParameter &set = static_cast<const SetParameter &>(msg);
         return handle(set);
         break;
      }
      case Message::SETPARAMETERCHOICES: {
         const SetParameterChoices &choice = static_cast<const SetParameterChoices &>(msg);
         return handle(choice);
         break;
      }
      case Message::PING: {
         const Ping &ping = static_cast<const Ping &>(msg);
         return handle(ping);
         break;
      }
      case Message::PONG: {
         const Pong &pong = static_cast<const Pong &>(msg);
         return handle(pong);
         break;
      }
      case Message::BUSY: {
         const Busy &busy = static_cast<const Busy &>(msg);
         return handle(busy);
         break;
      }
      case Message::IDLE: {
         const Idle &idle = static_cast<const Idle &>(msg);
         return handle(idle);
         break;
      }
      case Message::BARRIER: {
         const Barrier &barrier = static_cast<const Barrier &>(msg);
         return handle(barrier);
         break;
      }
      case Message::BARRIERREACHED: {
         const BarrierReached &reached = static_cast<const BarrierReached &>(msg);
         return handle(reached);
         break;
      }
      case Message::SETID: {
         const SetId &setid = static_cast<const SetId &>(msg);
         (void)setid;
         return true;
         break;
      }
      default:
         CERR << "message type not handled: type=" << msg.type() << std::endl;
         assert("message type not handled" == 0);
         break;
   }

   return false;
}

bool StateTracker::handle(const message::Ping &ping) {

   return true;
}

bool StateTracker::handle(const message::Pong &pong) {

   CERR << "Pong [" << pong.senderId() << " " << pong.getCharacter() << "]" << std::endl;
   return true;
}

bool StateTracker::handle(const message::Spawn &spawn) {

   int moduleId = spawn.spawnId();
   assert(moduleId > 0);

   Module &mod = runningMap[moduleId];
   mod.name = spawn.getName();

   for (StateObserver *o: m_observers) {
      o->newModule(moduleId, mod.name);
   }

   return true;
}

bool StateTracker::handle(const message::Started &started) {

   int moduleId = started.senderId();
   runningMap[moduleId].initialized = true;

   for (StateObserver *o: m_observers) {
      o->moduleStateChanged(moduleId, runningMap[moduleId].state());
   }

   return true;
}

bool StateTracker::handle(const message::Connect &connect) {

   portTracker()->addConnection(connect.getModuleA(),
         connect.getPortAName(),
         connect.getModuleB(),
         connect.getPortBName());

   for (StateObserver *o: m_observers) {
      o->newConnection(connect.getModuleA(), connect.getPortAName(),
            connect.getModuleB(), connect.getPortBName());
   }

   return true;
}

bool StateTracker::handle(const message::Disconnect &disconnect) {

   portTracker()->removeConnection(disconnect.getModuleA(),
         disconnect.getPortAName(),
         disconnect.getModuleB(),
         disconnect.getPortBName());

   for (StateObserver *o: m_observers) {
      o->deleteConnection(disconnect.getModuleA(), disconnect.getPortAName(),
            disconnect.getModuleB(), disconnect.getPortBName());
   }

   return true;
}

bool StateTracker::handle(const message::ModuleExit &moduleExit) {

   int mod = moduleExit.senderId();

   CERR << " Module [" << mod << "] quit" << std::endl;

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
   portTracker()->removeConnections(mod);

   for (StateObserver *o: m_observers) {
      o->deleteModule(mod);
   }

   return true;
}

bool StateTracker::handle(const message::Compute &compute) {

   return true;
}

bool StateTracker::handle(const message::Busy &busy) {

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

bool StateTracker::handle(const message::Idle &idle) {

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

bool StateTracker::handle(const message::AddParameter &addParam) {

#ifdef DEBUG
   CERR << "AddParameter: module=" << addParam.moduleName() << "(" << addParam.senderId() << "), name=" << addParam.getName() << std::endl;
#endif

   ParameterMap &pm = runningMap[addParam.senderId()].parameters;
   ParameterMap::iterator it = pm.find(addParam.getName());
   if (it != pm.end()) {
      CERR << "double parameter" << std::endl;
   } else {
      pm[addParam.getName()] = addParam.getParameter();
   }

   for (StateObserver *o: m_observers) {
      o->newParameter(addParam.senderId(), addParam.getName());
   }

   Port *p = portTracker()->addPort(new Port(addParam.senderId(), addParam.getName(), Port::PARAMETER));

   for (StateObserver *o: m_observers) {
      o->newPort(p->getModuleID(), p->getName());
   }

   return true;
}

bool StateTracker::handle(const message::SetParameter &setParam) {

#ifdef DEBUG
   CERR << "SetParameter: sender=" << setParam.senderId() << ", module=" << setParam.getModule() << ", name=" << setParam.getName() << std::endl;
#endif

   Parameter *param = getParameter(setParam.getModule(), setParam.getName());
   if (param)
      setParam.apply(param);

   for (StateObserver *o: m_observers) {
      o->parameterValueChanged(setParam.senderId(), setParam.getName());
   }

   return true;
}

bool StateTracker::handle(const message::SetParameterChoices &choices) {

   return true;
}

bool StateTracker::handle(const message::Kill &kill) {

   const int id = kill.getModule();
   runningMap[id].killed = true;

   for (StateObserver *o: m_observers) {
      o->moduleStateChanged(id, runningMap[id].state());
   }

   return true;
}

bool StateTracker::handle(const message::AddObject &addObj) {

   return true;
}

bool StateTracker::handle(const message::ObjectReceived &objRecv) {

   return true;
}

bool StateTracker::handle(const message::Barrier &barrier) {

   return true;
}

bool StateTracker::handle(const message::BarrierReached &barrReached) {

   return true;
}

bool StateTracker::handle(const message::CreatePort &createPort) {

   Port * p = portTracker()->addPort(createPort.getPort());

   for (StateObserver *o: m_observers) {
      o->newPort(p->getModuleID(), p->getName());
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

   const ParameterMap &pmap = rit->second.parameters;
   BOOST_FOREACH (ParameterMap::value_type val, pmap) {
      result.push_back(val.first);
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

void StateTracker::registerObserver(StateObserver *observer) {

   m_observers.insert(observer);
}

} // namespace vistle
