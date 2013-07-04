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
         break;
      }
      case Message::PONG: {
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

   int moduleID = spawn.spawnId();
   assert(moduleID > 0);

   Module &mod = runningMap[moduleID];
   mod.name = spawn.getName();

   return true;
}

bool StateTracker::handle(const message::Started &started) {

   int moduleID = started.senderId();
   runningMap[moduleID].initialized = true;
   assert(runningMap[moduleID].name == started.getName());

   return true;
}

bool StateTracker::handle(const message::Connect &connect) {

   portTracker()->addConnection(connect.getModuleA(),
         connect.getPortAName(),
         connect.getModuleB(),
         connect.getPortBName());

   return true;
}

bool StateTracker::handle(const message::Disconnect &disconnect) {

   portTracker()->removeConnection(disconnect.getModuleA(),
         disconnect.getPortAName(),
         disconnect.getModuleB(),
         disconnect.getPortBName());

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

   portTracker()->addPort(new Port(addParam.senderId(), addParam.getName(), Port::PARAMETER));

   return true;
}

bool StateTracker::handle(const message::SetParameter &setParam) {

#ifdef DEBUG
   CERR << "SetParameter: sender=" << setParam.senderId() << ", module=" << setParam.getModule() << ", name=" << setParam.getName() << std::endl;
#endif

   Parameter *param = getParameter(setParam.getModule(), setParam.getName());
   if (param)
      setParam.apply(param);

   return true;
}

bool StateTracker::handle(const message::SetParameterChoices &choices) {

   return true;
}

bool StateTracker::handle(const message::Kill &kill) {

   runningMap[kill.getModule()].killed = true;
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

   portTracker()->addPort(createPort.getPort());

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

} // namespace vistle
