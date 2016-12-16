#define ENUMS_FOR_PYTHON

#include <Python.h>
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
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

#ifdef VISTLE_CONTROL

// if embedded in Vistle hub
#include <hub/hub.h>
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

namespace bp = boost::python;
namespace asio = boost::asio;

BOOST_PYTHON_MODULE(vector_indexing_suite_ext){
   bp::class_<std::vector<int> >("PyVec")
      .def(bp::vector_indexing_suite<std::vector<int> >());
}

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
}

static void ping(int dest=message::Id::Broadcast, char c='.') {

#ifdef DEBUG
   std::cerr << "Python: ping: " << c << std::endl;
#endif
   message::Ping m(c);
   m.setDestId(dest);
   sendMessage(m);
}
BOOST_PYTHON_FUNCTION_OVERLOADS(ping_overloads, ping, 0, 2)

static void trace(int id=message::Id::Broadcast, message::Message::Type type=message::Message::ANY, bool onoff = true) {

#ifdef DEBUG
   auto cerrflags = std::cerr.flags();
   std::cerr << "Python: trace " << id << << ", type " << type << ": " << std::boolalpha << onoff << std::endl;
   std::cerr.flags(cerrflags);
#endif
   message::Trace m(id, type, onoff);
   sendMessage(m);
}
BOOST_PYTHON_FUNCTION_OVERLOADS(trace_overloads, trace, 0, 3)

static bool barrier() {
   message::Barrier m;
   m.setDestId(message::Id::MasterHub);
   MODULEMANAGER.registerRequest(m.uuid());
   if (!sendMessage(m))
      return false;
   auto buf = MODULEMANAGER.waitForReply(m.uuid());
   if (buf->type() == message::Message::BARRIERREACHED) {
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
BOOST_PYTHON_FUNCTION_OVERLOADS(spawnAsync_overloads, spawnAsync, 2, 5)

static std::string spawnAsyncSimple(const char *module, int numSpawn=-1, int baseRank=-1, int rankSkip=-1) {

   return spawnAsync(message::Id::MasterHub, module, numSpawn, baseRank, rankSkip);
}
BOOST_PYTHON_FUNCTION_OVERLOADS(spawnAsyncSimple_overloads, spawnAsyncSimple, 1, 4)

static int waitForSpawn(const std::string &uuid) {

   boost::uuids::string_generator gen;
   message::uuid_t u = gen(uuid);
   auto buf = MODULEMANAGER.waitForReply(u);
   if (buf->type() == message::Message::SPAWN) {
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
BOOST_PYTHON_FUNCTION_OVERLOADS(spawn_overloads, spawn, 2, 5)

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

static std::vector<int> waitForHubs(const std::vector<std::string> &names) {
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
      ret.push_back(a.name);
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
      std::cerr << "Python: getParameterType: no such parameter" << std::endl;
      return false;
   }

   return param->isDefault();
}

template<typename T>
static T getParameterValue(int id, const std::string &name) {

   LOCKED();
   const auto param = MODULEMANAGER.getParameter(id, name);
   if (!param) {
      std::cerr << "Python: getParameterValue: no such parameter" << std::endl;
      return T();
   }

   const auto tparam = std::dynamic_pointer_cast<const ParameterBase<T>>(param);
   if (!tparam) {
      std::cerr << "Python: getParameterValue: type mismatch" << std::endl;
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

static void setIntParam(int id, const char *name, Integer value) {

#ifdef DEBUG
   std::cerr << "Python: setIntParam " << id << ":" << name << " = " << value << std::endl;
#endif
   message::SetParameter m(id, name, value);
   m.setDestId(id);
   sendMessage(m);
}

static void setFloatParam(int id, const char *name, Float value) {

#ifdef DEBUG
   std::cerr << "Python: setFloatParam " << id << ":" << name << " = " << value << std::endl;
#endif
   message::SetParameter m(id, name, value);
   m.setDestId(id);
   sendMessage(m);
}

static void setVectorParam4(int id, const char *name, Float v1, Float v2, Float v3, Float v4) {

   message::SetParameter m(id, name, ParamVector(v1, v2, v3, v4));
   m.setDestId(id);
   sendMessage(m);
}

static void setVectorParam3(int id, const char *name, Float v1, Float v2, Float v3) {

   message::SetParameter m(id, name, ParamVector(v1, v2, v3));
   m.setDestId(id);
   sendMessage(m);
}

static void setVectorParam2(int id, const char *name, Float v1, Float v2) {

   message::SetParameter m(id, name, ParamVector(v1, v2));
   m.setDestId(id);
   sendMessage(m);
}

static void setVectorParam1(int id, const char *name, Float v1) {

   message::SetParameter m(id, name, ParamVector(v1));
   m.setDestId(id);
   sendMessage(m);
}

static void setIntVectorParam4(int id, const char *name, Integer v1, Integer v2, Integer v3, Integer v4) {

   message::SetParameter m(id, name, IntParamVector(v1, v2, v3, v4));
   m.setDestId(id);
   sendMessage(m);
}

static void setIntVectorParam3(int id, const char *name, Integer v1, Integer v2, Integer v3) {

   message::SetParameter m(id, name, IntParamVector(v1, v2, v3));
   m.setDestId(id);
   sendMessage(m);
}

static void setIntVectorParam2(int id, const char *name, Integer v1, Integer v2) {

   message::SetParameter m(id, name, IntParamVector(v1, v2));
   m.setDestId(id);
   sendMessage(m);
}

static void setIntVectorParam1(int id, const char *name, Integer v1) {

   message::SetParameter m(id, name, IntParamVector(v1));
   m.setDestId(id);
   sendMessage(m);
}

static void setStringParam(int id, const char *name, const std::string &value) {

#ifdef DEBUG
   std::cerr << "Python: setStringParam " << id << ":" << name << " = " << value << std::endl;
#endif
   message::SetParameter m(id, name, value);
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
BOOST_PYTHON_FUNCTION_OVERLOADS(compute_overloads, compute, 0, 1)

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
BOOST_PYTHON_FUNCTION_OVERLOADS(requestTunnel_overloads, requestTunnel, 2, 3)

static void removeTunnel(unsigned short listenPort) {
#ifdef DEBUG
   std::cerr << "Python: removeTunnel " << listenPort << std::endl;
#endif

   message::RequestTunnel m(listenPort);
   sendMessage(m);
}

#define param(T, f) \
   def("set" #T "Param", f, "set parameter `arg2` of module with ID `arg1` to `arg3`"); \
   def("setParam", f, "set parameter `arg2` of module with ID `arg1` to `arg3`");

BOOST_PYTHON_MODULE(_vistle)
{
    using namespace boost::python;

    // make values of vistle::message::Message::Type enum known to Python as Message.xxx
    vistle::message::Message::enumForPython_Type("Message");

    def("source", source, "execute commands from file `arg1`");
    def("spawn", spawn, spawn_overloads(args("hub", "modulename", "numspawn", "baserank", "rankskip"), "spawn new module `arg1`\n" "return its ID"));
    def("spawn", spawnSimple, "spawn new module `arg1`\n" "return its ID");
    def("spawnAsync", spawnAsync, spawnAsync_overloads(args("hub", "modulename", "numspawn", "baserank", "rankskip"), "spawn new module `arg1`\n" "return uuid to wait on its ID"));
    def("spawnAsync", spawnAsyncSimple, spawnAsyncSimple_overloads(args("modulename", "numspawn", "baserank", "rankskip"), "spawn new module `arg1`\n" "return uuid to wait on its ID"));
    def("waitForSpawn", waitForSpawn, "wait for asynchronously spawned module with uuid `arg1` and return its ID");
    def("kill", kill, "kill module with ID `arg1`");
    def("connect", connect, "connect output `arg2` of module with ID `arg1` to input `arg4` of module with ID `arg3`");
    def("disconnect", disconnect, "disconnect output `arg2` of module with ID `arg1` to input `arg4` of module with ID `arg3`");
    def("compute", compute, compute_overloads(args("module id"), "trigger execution of module with ID `arg1`"));
    def("quit", quit, "quit vistle session");
    def("ping", ping, ping_overloads(args("id", "data"), "send first character of `arg2` to destination `arg1`"));
    def("trace", trace, trace_overloads(args("id", "enable"), "enable/disable message tracing for module `arg1`"));
    def("barrier", barrier, "wait until all modules reply");
    def("requestTunnel", requestTunnel, requestTunnel_overloads(args("listen port", "dest port", "dest addr"), "start TCP tunnel listening on port `arg1` on hub forwarding incoming connections to `arg2`:`arg3`"));
    def("removeTunnel", removeTunnel, "remove TCP tunnel listening on port `arg1` on hub");
    //def("checkMessageQueue", checkMessageQueue, "check whether all messages have been processed");

    param(Int, setIntParam);
    param(Float, setFloatParam);
    param(String, setStringParam);
    param(Vector, setVectorParam1);
    param(Vector, setVectorParam2);
    param(Vector, setVectorParam3);
    param(Vector, setVectorParam4);
    param(IntVector, setIntVectorParam1);
    param(IntVector, setIntVectorParam2);
    param(IntVector, setIntVectorParam3);
    param(IntVector, setIntVectorParam4);

    def("getAvailable", getAvailable, "get list of names of available modules");
    def("getRunning", getRunning, "get list of IDs of running modules");
    def("getBusy", getBusy, "get list of IDs of busy modules");
    def("getModuleName", getModuleName, "get name of module with ID `arg1`");
    def("getInputPorts", getInputPorts, "get name of input ports of module with ID `arg1`");
    def("waitForHub", waitForNamedHub, "wait for slave hub named `arg1` to connect");
    def("waitForHub", waitForAnySlaveHub, "wait for any additional slave hub to connect");
    def("waitForHubs", waitForHubs, "wait for named hubs to connect");
    def("getMasterHub", getMasterHub, "get ID of master hub");
    def("getAllHubs", getAllHubs, "get ID of all known hubs");
    def("getHub", getHub, "get ID of hub for module with ID `arg1`");
    def("getOutputPorts", getOutputPorts, "get name of input ports of module with ID `arg1`");
    def("getConnections", getConnections, "get connections to/from port `arg2` of module with ID `arg1`");
    def("getParameters", getParameters, "get list of parameters for module with ID `arg1`");
    def("getParameterType", getParameterType, "get type of parameter named `arg2` of module with ID `arg1`");
    def("isParameterDefault", isParameterDefault, "check whether parameter `arg2` of module with ID `arg1` differs from its default value");
    def("getIntParam", getParameterValue<Integer>, "get value of parameter named `arg2` of module with ID `arg1`");
    def("getFloatParam", getParameterValue<Float>, "get value of parameter named `arg2` of module with ID `arg1`");
    def("getVectorParam", getParameterValue<ParamVector>, "get value of parameter named `arg2` of module with ID `arg1`");
    def("getIntVectorParam", getParameterValue<IntParamVector>, "get value of parameter named `arg2` of module with ID `arg1`");
    def("getStringParam", getParameterValue<std::string>, "get value of parameter named `arg2` of module with ID `arg1`");
}

PythonModule::PythonModule(const std::string &path)
: m_vistleConnection(nullptr)
{
   assert(s_instance == nullptr);
   s_instance = this;

   if (!import(&PythonInterface::the().nameSpace(), path)) {
      throw(vistle::except::exception("vistle python import failure"));
   }
}

PythonModule::PythonModule(VistleConnection *vc, const std::string &path)
   : m_vistleConnection(vc)
{
   assert(s_instance == nullptr);
   s_instance = this;

   if (!import(&PythonInterface::the().nameSpace(), path)) {
      throw(vistle::except::exception("vistle python import failure"));
   }
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

bool PythonModule::import(boost::python::object *ns, const std::string &path) {

   bp::class_<std::vector<int> >("vector<int>")
      .def(bp::vector_indexing_suite<std::vector<int> >());

   bp::class_<std::vector<std::string> >("vector<string>")
      .def(bp::vector_indexing_suite<std::vector<std::string> >());

   bp::class_<std::vector<std::pair<int, std::string> > >("vector<pair<int,string>>")
      .def(bp::vector_indexing_suite<std::vector<std::pair<int, std::string> > >());

   bp::class_<std::pair<int, std::string> >("pair<int,string>")
      .def_readwrite( "first", &std::pair< int, std::string >::first, "first value" )
      .def_readwrite( "second", &std::pair< int, std::string >::second, "second value" );

   bp::class_<ParameterVector<Float> >("ParameterVector<Float>")
      .def(bp::vector_indexing_suite<ParameterVector<Float> >());

   bp::class_<ParameterVector<Integer> >("ParameterVector<Integer>")
      .def(bp::vector_indexing_suite<ParameterVector<Integer> >());

   try {
#if PY_VERSION_HEX >= 0x03000000
      PyInit__vistle();
#else
      init_vistle();
#endif
   } catch (bp::error_already_set) {
      std::cerr << "vistle Python module initialisation failed: " << std::endl;
      if (PyErr_Occurred()) {
         std::cerr << PythonInterface::errorString() << std::endl;
      }
      bp::handle_exception();
      PyErr_Clear();
      return false;
   }

   // load boost::python wrapper - statically linked into binary
   try {
      (*ns)["_vistle"] = bp::import("_vistle");
   } catch (bp::error_already_set) {
      std::cerr << "vistle Python module import failed" << std::endl;
      if (PyErr_Occurred()) {
         std::cerr << PythonInterface::errorString() << std::endl;
      }
      bp::handle_exception();
      PyErr_Clear();
      return false;
   }

   // load vistle.py
   try {
      bp::dict locals;
      locals["modulename"] = "vistle";
      locals["path"] = path + "/vistle.py";
      bp::exec("import imp\n"
           "newmodule = imp.load_module(modulename, open(path), path, ('py', 'U', imp.PY_SOURCE))\n",
           *ns, locals);
      (*ns)["vistle"] = locals["newmodule"];
   } catch (bp::error_already_set) {
      std::cerr << "loading of vistle.py failed" << std::endl;
      if (PyErr_Occurred()) {
         std::cerr << PythonInterface::errorString() << std::endl;
      }
      bp::handle_exception();
      PyErr_Clear();
      return false;
   }
   if (!PythonInterface::the().exec("from vistle import *")) {
      std::cerr << "importing vistle.py Python add-on failed" << std::endl;
      PyErr_Print();
      return false;
   }

   return true;
}

} // namespace vistle
