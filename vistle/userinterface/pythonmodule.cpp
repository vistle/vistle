#define ENUMS_FOR_PYTHON

#include <cstdio>

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/eval.h>
#include <util/pybind.h>

#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>

#include <core/uuid.h>
#include <core/message.h>
#include <core/parameter.h>
#include <core/port.h>

#include "pythonmodule.h"
#include "pythoninterface.h"

//#define DEBUG

#include <core/statetracker.h>
#include <core/porttracker.h>

namespace py = pybind11;


#ifdef VISTLE_CONTROL

// if embedded in Vistle hub
#include <control/hub.h>
#define PORTMANAGER (*Hub::the().stateTracker().portTracker())
#define MODULEMANAGER (Hub::the().stateTracker())
#define LOCKED() StateTracker::mutex_locker locker(Hub::the().stateTracker().getMutex())

#else

// if part of a user interface
#include "vistleconnection.h"
#define PORTMANAGER (*(PythonModule::the().vistleConnection().ui().state().portTracker()))
#define MODULEMANAGER ((PythonModule::the().vistleConnection().ui().state()))
#define LOCKED() std::unique_ptr<vistle::VistleConnection::Locker> lock = PythonModule::the().vistleConnection().locked()

#endif

namespace asio = boost::asio;

namespace vistle {

PythonModule *PythonModule::s_instance = nullptr;

static bool sendMessage(const vistle::message::Message &m) {

#ifdef VISTLE_CONTROL
   bool ret = Hub::the().handleMessage(m);
   assert(ret);
   if (!ret) {
      std::cerr << "Python: failed to send message " << m << std::endl;
   }
   return ret;
#else
   return PythonModule::the().vistleConnection().sendMessage(m);
#endif
}

static void source(const std::string &filename) {

   PythonInterface::the().exec_file(filename);
}

static void quit() {

#ifdef DEBUG
   std::cerr << "Python: quit" << std::endl;
#endif
   message::Quit m;
   sendMessage(m);
   exit(0);
}

static void ping(int dest=message::Id::Broadcast, char c='.') {

#ifdef DEBUG
   std::cerr << "Python: ping: " << c << std::endl;
#endif
   message::Ping m(c);
   m.setDestId(dest);
   sendMessage(m);
}

static void trace(int id=message::Id::Broadcast, message::Type type=message::ANY, bool onoff = true) {

#ifdef DEBUG
   auto cerrflags = std::cerr.flags();
   std::cerr << "Python: trace " << id << << ", type " << type << ": " << std::boolalpha << onoff << std::endl;
   std::cerr.flags(cerrflags);
#endif
   message::Trace m(id, type, onoff);
   sendMessage(m);
}

static bool barrier() {
   message::Barrier m;
   m.setDestId(message::Id::MasterHub);
   MODULEMANAGER.registerRequest(m.uuid());
   if (!sendMessage(m))
      return false;
   auto buf = MODULEMANAGER.waitForReply(m.uuid());
   if (buf->type() == message::BARRIERREACHED) {
      return true;
   }
   return false;
}

static std::string spawnAsync(int hub, const char *module, int numSpawn=-1, int baseRank=-1, int rankSkip=-1) {

#ifdef DEBUG
   std::cerr << "Python: spawnAsync "<< module << std::endl;
#endif

   message::Spawn m(hub, module, numSpawn, baseRank, rankSkip);
   m.setDestId(message::Id::MasterHub); // to master for module id generation
   MODULEMANAGER.registerRequest(m.uuid());
   if (!sendMessage(m))
      return "";
   std::string uuid = boost::lexical_cast<std::string>(m.uuid());

   return uuid;
}

static std::string spawnAsyncSimple(const char *module, int numSpawn=-1, int baseRank=-1, int rankSkip=-1) {

   return spawnAsync(message::Id::MasterHub, module, numSpawn, baseRank, rankSkip);
}

static int waitForSpawn(const std::string &uuid) {

   boost::uuids::string_generator gen;
   message::uuid_t u = gen(uuid);
   auto buf = MODULEMANAGER.waitForReply(u);
   if (buf->type() == message::SPAWN) {
      auto &spawn = buf->as<message::Spawn>();
      return spawn.spawnId();
   } else {
      std::cerr << "waitForSpawn: got " << buf << std::endl;
   }

   return message::Id::Invalid;
}

static int spawn(int hub, const char *module, int numSpawn=-1, int baseRank=-1, int rankSkip=-1) {

#ifdef DEBUG
   std::cerr << "Python: spawn "<< module << std::endl;
#endif
   const std::string uuid = spawnAsync(hub, module, numSpawn, baseRank, rankSkip);
   return waitForSpawn(uuid);
}

static int spawnSimple(const char *module) {

   return spawn(message::Id::MasterHub, module);
}

static void kill(int id) {

#ifdef DEBUG
   std::cerr << "Python: kill "<< id << std::endl;
#endif
   message::Kill m(id);
   m.setDestId(id);
   sendMessage(m);
}

static int waitForAnySlaveHub() {
   auto hubs = MODULEMANAGER.getSlaveHubs();
   if (!hubs.empty())
      return hubs[0];

   hubs = MODULEMANAGER.waitForSlaveHubs(1);
   if (!hubs.empty())
      return hubs[0];

   return message::Id::Invalid;
}

static std::vector<int> waitForSlaveHubs(int count) {
   auto hubs = MODULEMANAGER.waitForSlaveHubs(count);
   return hubs;
}

static int waitForNamedHub(const std::string &name) {
   std::vector<std::string> names;
   names.push_back(name);
   const auto ids = MODULEMANAGER.waitForSlaveHubs(names);
   for (const auto id: ids) {
      const auto n = MODULEMANAGER.hubName(id);
      if (n == name)
      {
         return id;
      }
   }
   return message::Id::Invalid;
}

static std::vector<int> waitForNamedHubs(const std::vector<std::string> &names) {
   return MODULEMANAGER.waitForSlaveHubs(names);
}

static int getHub(int id) {

   LOCKED();
   return MODULEMANAGER.getHub(id);
}

static int getMasterHub() {

   LOCKED();
   return MODULEMANAGER.getMasterHub();
}

static int getVistleSession() {
    return vistle::message::Id::Vistle;
}

static std::vector<int> getAllHubs() {

   LOCKED();
   return MODULEMANAGER.getSlaveHubs();
}

static std::vector<int> getRunning() {

   LOCKED();
#ifdef DEBUG
   std::cerr << "Python: getRunning " << std::endl;
#endif
   return MODULEMANAGER.getRunningList();
}

static std::vector<std::string> getAvailable() {

   LOCKED();
   const auto &avail = MODULEMANAGER.availableModules();
   std::vector<std::string> ret;
   for (auto &a: avail) {
      auto &m = a.second;
      ret.push_back(m.name);
   }
   return ret;
}

static std::vector<int> getBusy() {

   LOCKED();
#ifdef DEBUG
   std::cerr << "Python: getBusy " << std::endl;
#endif
   return MODULEMANAGER.getBusyList();
}

static std::vector<std::string> getInputPorts(int id) {

   LOCKED();
   return PORTMANAGER.getInputPortNames(id);
}

static std::vector<std::string> getOutputPorts(int id) {

   LOCKED();
   return PORTMANAGER.getOutputPortNames(id);
}

static std::vector<std::pair<int, std::string> > getConnections(int id, const std::string &port) {

   LOCKED();
   std::vector<std::pair<int, std::string> > result;

   if (const Port::ConstPortSet *c = PORTMANAGER.getConnectionList(id, port)) {
      for (const Port *p: *c) {
         result.push_back(std::pair<int, std::string>(p->getModuleID(), p->getName()));
      }
   }

   return result;
}

static std::vector<std::string> getParameters(int id) {

   LOCKED();
   return MODULEMANAGER.getParameters(id);
}

static std::string getParameterType(int id, const std::string &name) {

   LOCKED();
   const auto param = MODULEMANAGER.getParameter(id, name);
   if (!param) {
      std::cerr << "Python: getParameterType: no such parameter" << std::endl;
      return "None";
   }

   switch (param->type()) {
      case Parameter::Integer: return "Int";
      case Parameter::Float: return "Float";
      case Parameter::Vector: return "Vector";
      case Parameter::IntVector: return "IntVector";
      case Parameter::String: return "String";
      case Parameter::Invalid: return "None";
      case Parameter::Unknown: return "None";
   }

   return "None";
}

static bool isParameterDefault(int id, const std::string &name) {

   LOCKED();
   const auto param = MODULEMANAGER.getParameter(id, name);
   if (!param) {
      std::cerr << "Python: getParameterType: no such parameter: id=" << id << ", name=" << name << std::endl;
      return false;
   }

   return param->isDefault();
}

template<typename T>
static T getParameterValue(int id, const std::string &name) {

   LOCKED();
   const auto param = MODULEMANAGER.getParameter(id, name);
   if (!param) {
      std::cerr << "Python: getParameterValue: no such parameter: id=" << id << ", name=" << name << std::endl;
      return T();
   }

   const auto tparam = std::dynamic_pointer_cast<const ParameterBase<T>>(param);
   if (!tparam) {
      std::cerr << "Python: getParameterValue: type mismatch: id=" << id << ", name=" << name << std::endl;
      return T();
   }

   return tparam->getValue();
}

static std::string getModuleName(int id) {

   LOCKED();
#ifdef DEBUG
   std::cerr << "Python: getModuleName(" id << ")" << std::endl;
#endif
   return MODULEMANAGER.getModuleName(id);
}

static void connect(int sid, const char *sport, int did, const char *dport) {

#ifdef DEBUG
   std::cerr << "Python: connect "<< sid << ":" << sport << " -> " << did << ":" << dport << std::endl;
#endif
   message::Connect m(sid, sport, did, dport);
   sendMessage(m);
}

static void disconnect(int sid, const char *sport, int did, const char *dport) {

#ifdef DEBUG
   std::cerr << "Python: disconnect "<< sid << ":" << sport << " -> " << did << ":" << dport << std::endl;
#endif
   message::Disconnect m(sid, sport, did, dport);
   sendMessage(m);
}

static void setIntParam(int id, const char *name, Integer value, bool delayed) {

#ifdef DEBUG
   std::cerr << "Python: setIntParam " << id << ":" << name << " = " << value << std::endl;
#endif
   message::SetParameter m(id, name, value);
   m.setDestId(id);
   if (delayed)
       m.setDelayed();
   sendMessage(m);
}

static void setFloatParam(int id, const char *name, Float value, bool delayed) {

#ifdef DEBUG
   std::cerr << "Python: setFloatParam " << id << ":" << name << " = " << value << std::endl;
#endif
   message::SetParameter m(id, name, value);
   m.setDestId(id);
   if (delayed)
       m.setDelayed();
   sendMessage(m);
}

static void setVectorParam4(int id, const char *name, Float v1, Float v2, Float v3, Float v4, bool delayed) {

   message::SetParameter m(id, name, ParamVector(v1, v2, v3, v4));
   m.setDestId(id);
   if (delayed)
       m.setDelayed();
   sendMessage(m);
}

static void setVectorParam3(int id, const char *name, Float v1, Float v2, Float v3, bool delayed) {

   message::SetParameter m(id, name, ParamVector(v1, v2, v3));
   m.setDestId(id);
   if (delayed)
       m.setDelayed();
   sendMessage(m);
}

static void setVectorParam2(int id, const char *name, Float v1, Float v2, bool delayed) {

   message::SetParameter m(id, name, ParamVector(v1, v2));
   m.setDestId(id);
   if (delayed)
       m.setDelayed();
   sendMessage(m);
}

static void setVectorParam1(int id, const char *name, Float v1, bool delayed) {

   message::SetParameter m(id, name, ParamVector(v1));
   m.setDestId(id);
   if (delayed)
       m.setDelayed();
   sendMessage(m);
}

static void setIntVectorParam4(int id, const char *name, Integer v1, Integer v2, Integer v3, Integer v4, bool delayed) {

   message::SetParameter m(id, name, IntParamVector(v1, v2, v3, v4));
   m.setDestId(id);
   if (delayed)
       m.setDelayed();
   sendMessage(m);
}

static void setIntVectorParam3(int id, const char *name, Integer v1, Integer v2, Integer v3, bool delayed) {

   message::SetParameter m(id, name, IntParamVector(v1, v2, v3));
   m.setDestId(id);
   if (delayed)
       m.setDelayed();
   sendMessage(m);
}

static void setIntVectorParam2(int id, const char *name, Integer v1, Integer v2, bool delayed) {

   message::SetParameter m(id, name, IntParamVector(v1, v2));
   m.setDestId(id);
   if (delayed)
       m.setDelayed();
   sendMessage(m);
}

static void setIntVectorParam1(int id, const char *name, Integer v1, bool delayed) {

   message::SetParameter m(id, name, IntParamVector(v1));
   m.setDestId(id);
   if (delayed)
       m.setDelayed();
   sendMessage(m);
}

static void setStringParam(int id, const char *name, const std::string &value, bool delayed) {

#ifdef DEBUG
   std::cerr << "Python: setStringParam " << id << ":" << name << " = " << value << std::endl;
#endif
   message::SetParameter m(id, name, value);
   m.setDestId(id);
   if (delayed)
       m.setDelayed();
   sendMessage(m);
}

static void applyParameters(int id) {

#ifdef DEBUG
   std::cerr << "Python: applyParameters " << id << std::endl;
#endif
   message::SetParameter m(id);
   m.setDestId(id);
   sendMessage(m);
}

static void compute(int id=message::Id::Broadcast) {

#ifdef DEBUG
   std::cerr << "Python: compute " << id << std::endl;
#endif
   message::Execute m(message::Execute::ComputeExecute, id);
   if (id == message::Id::Broadcast)
      m.setDestId(message::Id::MasterHub);
   else
      m.setDestId(id);
   sendMessage(m);
}

static void cancelCompute(int id) {
#ifdef DEBUG
   std::cerr << "Python: cancelCompute " << id << std::endl;
#endif
   message::CancelExecute m(id);
   m.setDestId(id);
   sendMessage(m);
}

static void requestTunnel(unsigned short listenPort, const std::string &destHost, unsigned short destPort=0) {
#ifdef DEBUG
   std::cerr << "Python: requestTunnel " << listenPort << " -> " << destHost << ":" << destPort << std::endl;
#endif
   if (destPort == 0)
      destPort = listenPort;

   message::RequestTunnel m(listenPort, destHost, destPort);

   asio::io_service io_service;
   asio::ip::tcp::resolver resolver(io_service);
   try {
      auto endpoints = resolver.resolve({destHost, boost::lexical_cast<std::string>(destPort)});
      auto addr = (*endpoints).endpoint().address();
      if (addr.is_v6()) {
         m.setDestAddr(addr.to_v6());
         std::cerr << destHost << " resolved to " << addr.to_v6() << std::endl;
      } else if (addr.is_v4()) {
         m.setDestAddr(addr.to_v4());
         std::cerr << destHost << " resolved to " << addr.to_v4() << std::endl;
      }
   } catch(...) {
   }

   sendMessage(m);
}

static void removeTunnel(unsigned short listenPort) {
#ifdef DEBUG
   std::cerr << "Python: removeTunnel " << listenPort << std::endl;
#endif

   message::RequestTunnel m(listenPort);
   sendMessage(m);
}

static void printInfo(const std::string &message) {
#ifdef DEBUG
   std::cerr << "Python: printInfo " << message << std::endl;
#endif

   message::SendText m(message::SendText::Info, message);
   sendMessage(m);
}

static void printWarning(const std::string &message) {
#ifdef DEBUG
   std::cerr << "Python: printWarning " << message << std::endl;
#endif

   message::SendText m(message::SendText::Warning, message);
   sendMessage(m);
}

static void printError(const std::string &message) {
#ifdef DEBUG
   std::cerr << "Python: printError " << message << std::endl;
#endif

   message::SendText m(message::SendText::Error, message);
   sendMessage(m);
}

static void setStatus(const std::string &text, message::UpdateStatus::Importance prio) {

   message::UpdateStatus m(text, prio);
   sendMessage(m);
}

static void clearStatus() {

    setStatus(std::string(), message::UpdateStatus::Bulk);
}

static void setLoadedFile(const std::string &file) {

   message::UpdateStatus m(message::UpdateStatus::LoadedFile, file);
   sendMessage(m);
}

static std::string getLoadedFile() {

   return MODULEMANAGER.loadedWorkflowFile();
}


#define param1(T, f) \
   m.def("set" #T "Param", &f, "set parameter `name` of module with `id` to `value`", "id"_a, "name"_a, "value"_a, "delayed"_a=false); \
   m.def("setParam", &f, "set parameter `name` of module with `id` to `value`", "id"_a, "name"_a, "value"_a, "delayed"_a=false);

#define param2(T, f) \
   m.def("set" #T "Param", &f, "set parameter `name` of module with `id` to (`value1`, `value2`)", "id"_a, "name"_a, "value1"_a, "value2"_a, "delayed"_a=false); \
   m.def("setParam", &f, "set parameter `name` of module with `id` to `(value1, value2)`", "id"_a, "name"_a, "value1"_a, "value2"_a, "delayed"_a=false);

#define param3(T, f) \
   m.def("set" #T "Param", &f, "set parameter `name` of module with `id` to (`value1`, `value2`, `value3`)", "id"_a, "name"_a, "value1"_a, "value2"_a, "value3"_a, "delayed"_a=false); \
   m.def("setParam", &f, "set parameter `name` of module with `id` to `(value1, value2, `value3`)", "id"_a, "name"_a, "value1"_a, "value2"_a, "value3"_a, "delayed"_a=false);

#define param4(T, f) \
   m.def("set" #T "Param", &f, "set parameter `name` of module with `id` to (`value1`, `value2`, `value3`, `value4`)", "id"_a, "name"_a, "value1"_a, "value2"_a, "value3"_a, "value4"_a, "delayed"_a=false); \
   m.def("setParam", &f, "set parameter `name` of module with `id` to `(value1, value2, `value3`, `value4`)", "id"_a, "name"_a, "value1"_a, "value2"_a, "value3"_a, "value4"_a, "delayed"_a=false);

PYBIND11_EMBEDDED_MODULE(_vistle, m) {

    using namespace py::literals;
    m.doc() = "Vistle Python bindings";

    // make values of vistle::message::Type enum known to Python as Message.xxx
    py::class_<message::Message> message(m, "Message");
    vistle::message::enumForPython_Type(message, "Message");

    // make values of vistle::message::UpdateStatus::Importance enum known to Python as Importance.xxx
    py::class_<message::UpdateStatus> us(m, "Status");
    vistle::message::UpdateStatus::enumForPython_Importance(us, "Importance");

    py::class_<message::Id> id(m, "Id");
    py::enum_<message::Id::Reserved>(id, "Id")
            .value("Invalid", message::Id::Invalid)
            .value("Vistle", message::Id::Vistle)
            .value("Broadcast", message::Id::Broadcast)
            .value("ForBroadcast", message::Id::ForBroadcast)
            .value("NextHop", message::Id::NextHop)
            .value("UI", message::Id::UI)
            .value("LocalManager", message::Id::LocalManager)
            .value("LocalHub", message::Id::LocalHub)
            .value("MasterHub", message::Id::MasterHub)
            .export_values();

    m.def("source", &source, "execute commands from `file`", "file"_a);
    m.def("spawn", spawn, "spawn new module `arg1`\n" "return its ID",
          "hub"_a, "modulename"_a, "numspawn"_a=-1, "baserank"_a=-1, "rankskip"_a=-1);
    m.def("spawn", spawnSimple, "spawn new module `arg1`\n" "return its ID");
    m.def("spawnAsync", spawnAsync, "spawn new module `arg1`\n" "return uuid to wait on its ID",
          "hub"_a, "modulename"_a, "numspawn"_a=-1, "baserank"_a=-1, "rankskip"_a=-1);
    m.def("spawnAsync", spawnAsyncSimple, "spawn new module `arg1`\n" "return uuid to wait on its ID",
          "modulename"_a, "numspawn"_a=1, "baserank"_a=-1, "rankskip"_a=-1);
    m.def("waitForSpawn", waitForSpawn, "wait for asynchronously spawned module with uuid `arg1` and return its ID");
    m.def("kill", kill, "kill module with ID `arg1`");
    m.def("connect", connect, "connect output `arg2` of module with ID `arg1` to input `arg4` of module with ID `arg3`");
    m.def("disconnect", disconnect, "disconnect output `arg2` of module with `id1` to input `arg4` of module with `id2`");
    m.def("compute", compute, "trigger execution of module with `id`", "moduleId"_a=message::Id::Broadcast);
    m.def("interrupt", cancelCompute, "interrupt execution of module with ID `arg1`");
    m.def("quit", quit, "quit vistle session");
    m.def("ping", ping, "send first character of `arg2` to destination `arg1`",
          "id"_a, "data"_a="p");
    m.def("trace", trace, "enable/disable message tracing for module `id`",
          "id"_a=message::Id::Broadcast, "type"_a=message::ANY, "enable"_a=true);
    m.def("barrier", barrier, "wait until all modules reply");
    m.def("requestTunnel", requestTunnel, "start TCP tunnel listening on port `arg1` on hub forwarding incoming connections to `arg2`:`arg3`",
          "listen port"_a, "dest port"_a, "dest addr"_a);
    m.def("removeTunnel", removeTunnel, "remove TCP tunnel listening on port `arg1` on hub");
    //def("checkMessageQueue", checkMessageQueue, "check whether all messages have been processed");
    m.def("printInfo", printInfo, "show info message to user");
    m.def("printWarning", printWarning, "show warning message to user");
    m.def("printError", printError, "show error message to user");
    m.def("setStatus", setStatus, "update status information", "text"_a, "importance"_a=message::UpdateStatus::Low);
    //m.def("setStatus", setStatus, "update status information");
    m.def("clearStatus", clearStatus, "clear status information");
    m.def("setLoadedFile", setLoadedFile, "update name of currently loaded workflow description file");
    m.def("getLoadedFile", getLoadedFile, "name of currently loaded workflow description file");

    param1(Int, setIntParam);
    param1(Float, setFloatParam);
    param1(String, setStringParam);
    param1(Vector, setVectorParam1);
    param2(Vector, setVectorParam2);
    param3(Vector, setVectorParam3);
    param4(Vector, setVectorParam4);
    param1(IntVector, setIntVectorParam1);
    param2(IntVector, setIntVectorParam2);
    param3(IntVector, setIntVectorParam3);
    param4(IntVector, setIntVectorParam4);

    m.def("applyParameters", applyParameters, "apply delayed parameter changes");
    m.def("getAvailable", getAvailable, "get list of names of available modules");
    m.def("getRunning", getRunning, "get list of IDs of running modules");
    m.def("getBusy", getBusy, "get list of IDs of busy modules");
    m.def("getModuleName", getModuleName, "get name of module with ID `arg1`");
    m.def("getInputPorts", getInputPorts, "get name of input ports of module with ID `arg1`");
    m.def("waitForHub", waitForNamedHub, "wait for slave hub named `arg1` to connect");
    m.def("waitForHub", waitForAnySlaveHub, "wait for any additional slave hub to connect");
    m.def("waitForHubs", waitForSlaveHubs, "wait for `count` additional slave hubs to connect");
    m.def("waitForNamedHubs", waitForNamedHubs, "wait for named hubs to connect");
    m.def("getMasterHub", getMasterHub, "get ID of master hub");
    m.def("getVistleSession", getVistleSession, "get ID for Vistle session");
    m.def("getAllHubs", getAllHubs, "get ID of all known hubs");
    m.def("getHub", getHub, "get ID of hub for module with ID `arg1`");
    m.def("getOutputPorts", getOutputPorts, "get name of input ports of module with ID `arg1`");
    m.def("getConnections", getConnections, "get connections to/from port `arg2` of module with ID `arg1`");
    m.def("getParameters", getParameters, "get list of parameters for module with ID `arg1`");
    m.def("getParameterType", getParameterType, "get type of parameter named `arg2` of module with ID `arg1`");
    m.def("isParameterDefault", isParameterDefault, "check whether parameter `arg2` of module with ID `arg1` differs from its default value");
    m.def("getIntParam", getParameterValue<Integer>, "get value of parameter named `arg2` of module with ID `arg1`");
    m.def("getFloatParam", getParameterValue<Float>, "get value of parameter named `arg2` of module with ID `arg1`");
    m.def("getVectorParam", getParameterValue<ParamVector>, "get value of parameter named `arg2` of module with ID `arg1`");
    m.def("getIntVectorParam", getParameterValue<IntParamVector>, "get value of parameter named `arg2` of module with ID `arg1`");
    m.def("getStringParam", getParameterValue<std::string>, "get value of parameter named `arg2` of module with ID `arg1`");

   py::bind_vector<ParameterVector<Float>>(m, "ParameterVector<Float>");
   py::bind_vector<ParameterVector<Integer>>(m, "ParameterVector<Integer>");
}

PythonModule::PythonModule(VistleConnection *vc)
   : m_vistleConnection(vc)
{
   assert(s_instance == nullptr);
   s_instance = this;
   std::cerr << "creating Vistle python module" << std::endl;

#if 0
#if PY_VERSION_HEX >= 0x03000000
   PyImport_AppendInittab("_vistle", PyInit__vistle);
#else
   PyImport_AppendInittab("_vistle", init_vistle);
#endif
#else
   //auto mod = py::module::import("_vistle");
#endif
}

PythonModule &PythonModule::the()
{
   assert(s_instance);
   return *s_instance;
}

VistleConnection &PythonModule::vistleConnection() const
{
   assert(m_vistleConnection);
   return *m_vistleConnection;
}

bool PythonModule::import(py::object *ns, const std::string &path) {

#if 0
   // load boost::python wrapper - statically linked into binary
   try {
      (*ns)["_vistle"] = py::module::import("_vistle");
   } catch (py::error_already_set) {
      std::cerr << "vistle Python module import failed" << std::endl;
      if (PyErr_Occurred()) {
         std::cerr << PythonInterface::errorString() << std::endl;
      }
      //py::handle_exception();
      PyErr_Clear();
      return false;
   }
#endif

   // load vistle.py
   try {
      py::dict locals;
      locals["modulename"] = "vistle";
      locals["path"] = path + "/vistle.py";
      std::cerr << "Python: loading " << path + "/vistle.py" << std::endl;
#if PY_MAJOR_VERSION > 3 || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 6)
      py::eval<py::eval_statements>(R"(
         import importlib
         spec = importlib.util.spec_from_file_location(modulename, path)
         if spec != None and spec.loader != None:
             newmodule = spec.loader.load_module()
         )", *ns, locals);
#else
      py::eval<py::eval_statements>(R"(
         import imp
         newmodule = imp.load_module(modulename, open(path), path, ('py', 'U', imp.PY_SOURCE))
         )", *ns, locals);
#endif
      (*ns)["vistle"] = locals["newmodule"];
   } catch (py::error_already_set &) {
      std::cerr << "loading of vistle.py failed" << std::endl;
      if (PyErr_Occurred()) {
         std::cerr << PythonInterface::errorString() << std::endl;
      }
      //py::handle_exception();
      PyErr_Clear();
      return false;
   }
   if (!PythonInterface::the().exec("from vistle import *")) {
      std::cerr << "importing vistle.py Python add-on failed" << std::endl;
      PyErr_Print();
      return false;
   }

   std::cerr << "done loading of vistle.py" << std::endl;
   return true;
}

} // namespace vistle
