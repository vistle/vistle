#include <Python.h>
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <core/message.h>
#include <core/parameter.h>

#include "communicator.h"
#include "pythonmodule.h"
#include "pythonembed.h"

//#define DEBUG

namespace bp = boost::python;

BOOST_PYTHON_MODULE(vector_indexing_suite_ext){
   bp::class_<std::vector<int> >("PyVec")
      .def(bp::vector_indexing_suite<std::vector<int> >());
}

namespace vistle {

static void print_output(const std::string &str) {

   PythonEmbed::print_output(str);
}

static void print_error(const std::string &str) {

   PythonEmbed::print_error(str);
}

static std::string raw_input(const std::string &prompt) {

   return PythonEmbed::raw_input(prompt);
}

static std::string readline() {

   return PythonEmbed::readline();
}

static void resetModuleCounter() {

   Communicator::the().resetModuleCounter();
}

static void source(const std::string &filename) {

   PythonEmbed::the().exec_file(filename);
}

static void quit() {

#ifdef DEBUG
   std::cerr << "Python: quit" << std::endl;
#endif
   message::Quit m;
   PythonEmbed::handleMessage(m);
   Communicator::the().setQuitFlag();
}

static void ping(char c) {

#ifdef DEBUG
   std::cerr << "Python: ping: " << c << std::endl;
#endif
   message::Ping m(c);
   PythonEmbed::handleMessage(m);
}

static int barrier() {

   boost::unique_lock<boost::mutex> lock(Communicator::the().barrierMutex());
   const int id = Communicator::the().getBarrierCounter();
   message::Barrier m(id);
   PythonEmbed::handleMessage(m);
   Communicator::the().barrierCondition().wait(lock);
   return id;
}

static int spawn(const char *module, int debugflag=0, int debugrank=0) {

#ifdef DEBUG
   std::cerr << "Python: spawn "<< module << std::endl;
#endif
   int id = Communicator::the().newModuleID();
   message::Spawn m(id, module, debugflag, debugrank);
   PythonEmbed::handleMessage(m);
   return id;
}
BOOST_PYTHON_FUNCTION_OVERLOADS(spawn_overloads, spawn, 1, 3)

static void kill(int id) {

#ifdef DEBUG
   std::cerr << "Python: kill "<< id << std::endl;
#endif
   message::Kill m(id);
   PythonEmbed::handleMessage(m);
}

static std::vector<int> getRunning() {

#ifdef DEBUG
   std::cerr << "Python: getRunning " << std::endl;
#endif
   return Communicator::the().getRunningList();
}

static std::vector<int> getBusy() {

#ifdef DEBUG
   std::cerr << "Python: getBusy " << std::endl;
#endif
   return Communicator::the().getBusyList();
}

static std::vector<std::string> getInputPorts(int id) {

   return Communicator::the().portManager().getInputPortNames(id);
}

static std::vector<std::string> getOutputPorts(int id) {

   return Communicator::the().portManager().getOutputPortNames(id);
}

static std::vector<std::pair<int, std::string> > getConnections(int id, const std::string &port) {

   std::vector<std::pair<int, std::string> > result;

   const PortManager::ConnectionList *c = Communicator::the().portManager().getConnectionList(id, port);
   for (size_t i=0; i<c->size(); ++i) {
      const Port *p = c->at(i);
      result.push_back(std::pair<int, std::string>(p->getModuleID(), p->getName()));
   }

   return result;
}

static std::vector<std::string> getParameters(int id) {

   return Communicator::the().getParameters(id);
}

static std::string getParameterType(int id, const std::string &name) {

   const Parameter *param = Communicator::the().getParameter(id, name);
   if (!param) {
      std::cerr << "Python: getParameterType: no such parameter" << std::endl;
      return "None";
   }

   switch (param->type()) {
      case Parameter::Integer: return "Int";
      case Parameter::Scalar: return "Float";
      case Parameter::Vector: return "Vector";
      case Parameter::String: return "String";
      case Parameter::Invalid: return "None";
      case Parameter::Unknown: return "None";
   }

   return "None";
}

static bool isParameterDefault(int id, const std::string &name) {

   const Parameter *param = Communicator::the().getParameter(id, name);
   if (!param) {
      std::cerr << "Python: getParameterType: no such parameter" << std::endl;
      return false;
   }

   return param->isDefault();
}

template<typename T>
static T getParameterValue(int id, const std::string &name) {

   const Parameter *param = Communicator::the().getParameter(id, name);
   if (!param) {
      std::cerr << "Python: getParameterValue: no such parameter" << std::endl;
      return T();
   }

   const ParameterBase<T> *tparam = dynamic_cast<const ParameterBase<T> *>(param);
   if (!tparam) {
      std::cerr << "Python: getParameterValue: type mismatch" << std::endl;
      return T();
   }

   return tparam->getValue();
}

static std::string getModuleName(int id) {

#ifdef DEBUG
   std::cerr << "Python: getModuleName(" id << ")" << std::endl;
#endif
   return Communicator::the().getModuleName(id);
}

static void connect(int sid, const char *sport, int did, const char *dport) {

#ifdef DEBUG
   std::cerr << "Python: connect "<< sid << ":" << sport << " -> " << did << ":" << dport << std::endl;
#endif
   message::Connect m(sid, sport, did, dport);
   PythonEmbed::handleMessage(m);
}

static void disconnect(int sid, const char *sport, int did, const char *dport) {

#ifdef DEBUG
   std::cerr << "Python: disconnect "<< sid << ":" << sport << " -> " << did << ":" << dport << std::endl;
#endif
   message::Disconnect m(sid, sport, did, dport);
   PythonEmbed::handleMessage(m);
}

static void setIntParam(int id, const char *name, int value) {

#ifdef DEBUG
   std::cerr << "Python: setIntParam " << id << ":" << name << " = " << value << std::endl;
#endif
   message::SetParameter m(id, name, value);
   PythonEmbed::handleMessage(m);
}

static void setFloatParam(int id, const char *name, double value) {

#ifdef DEBUG
   std::cerr << "Python: setFloatParam " << id << ":" << name << " = " << value << std::endl;
#endif
   message::SetParameter m(id, name, value);
   PythonEmbed::handleMessage(m);
}

static void setVectorParam4(int id, const char *name, double v1, double v2, double v3, double v4) {

   message::SetParameter m(id, name, ParamVector(v1, v2, v3, v4));
   PythonEmbed::handleMessage(m);
}

static void setVectorParam3(int id, const char *name, double v1, double v2, double v3) {

   message::SetParameter m(id, name, ParamVector(v1, v2, v3));
   PythonEmbed::handleMessage(m);
}

static void setVectorParam2(int id, const char *name, double v1, double v2) {

   message::SetParameter m(id, name, ParamVector(v1, v2));
   PythonEmbed::handleMessage(m);
}

static void setVectorParam1(int id, const char *name, double v1) {

   message::SetParameter m(id, name, ParamVector(v1));
   PythonEmbed::handleMessage(m);
}

static void setStringParam(int id, const char *name, const std::string &value) {

#ifdef DEBUG
   std::cerr << "Python: setStringParam " << id << ":" << name << " = " << value << std::endl;
#endif
   message::SetParameter m(id, name, value);
   PythonEmbed::handleMessage(m);
}

static bool checkMessageQueue() {

   return Communicator::the().checkMessageQueue();
}

static void compute(int id) {

#ifdef DEBUG
   std::cerr << "Python: compute " << id << std::endl;
#endif
   int count = Communicator::the().newExecutionCount();
   message::Compute m(id, count);
   PythonEmbed::handleMessage(m);
}

#define param(T, f) \
   def("set" #T "Param", f, "set parameter `arg2` of module with ID `arg1` to `arg3`"); \
   def("setParam", f, "set parameter `arg2` of module with ID `arg1` to `arg3`");

BOOST_PYTHON_MODULE(_vistle)
{
    using namespace boost::python;
    def("_readline", readline);
    def("_raw_input", raw_input);
    def("_print_error", print_error);
    def("_print_output", print_output);

    def("_resetModuleCounter", resetModuleCounter);

    def("source", source, "execute commands from file `arg1`");
    def("spawn", spawn, spawn_overloads(args("modulename", "debug", "debugrank"), "spawn new module `arg1`\n" "return its ID"));
    def("kill", kill, "kill module with ID `arg1`");
    def("connect", connect, "connect output `arg2` of module with ID `arg1` to input `arg4` of module with ID `arg3`");
    def("compute", compute, "trigger execution of module with ID `arg1`");
    def("quit", quit, "quit vistle session");
    def("ping", ping, "send first character of `arg1` to every vistle process");
    def("barrier", barrier, "wait until all modules reply");
    def("checkMessageQueue", checkMessageQueue, "check whether all messages have been processed");

    param(Int, setIntParam);
    param(Float, setFloatParam);
    param(String, setStringParam);
    param(Vector, setVectorParam1);
    param(Vector, setVectorParam2);
    param(Vector, setVectorParam3);
    param(Vector, setVectorParam4);

    def("getRunning", getRunning, "get list of IDs of running modules");
    def("getBusy", getBusy, "get list of IDs of busy modules");
    def("getModuleName", getModuleName, "get name of module with ID `arg1`");
    def("getInputPorts", getInputPorts, "get name of input ports of module with ID `arg1`");
    def("getOutputPorts", getOutputPorts, "get name of input ports of module with ID `arg1`");
    def("getConnections", getConnections, "get connections to/from port `arg2` of module with ID `arg1`");
    def("getParameters", getParameters, "get list of parameters for module with ID `arg1`");
    def("getParameterType", getParameterType, "get type of parameter named `arg2` of module with ID `arg1`");
    def("isParameterDefault", isParameterDefault, "check whether parameter `arg2` of module with ID `arg1` differs from its default value");
    def("getIntParam", getParameterValue<int>, "get value of parameter named `arg2` of module with ID `arg1`");
    def("getFloatParam", getParameterValue<Scalar>, "get value of parameter named `arg2` of module with ID `arg1`");
    def("getVectorParam", getParameterValue<ParamVector>, "get value of parameter named `arg2` of module with ID `arg1`");
    def("getStringParam", getParameterValue<std::string>, "get value of parameter named `arg2` of module with ID `arg1`");
}

bool PythonModule::import(boost::python::object *ns) {

   try {
      init_vistle();
   } catch (bp::error_already_set) {
      std::cerr << "vistle Python module initialisation failed" << std::endl;
      PyErr_Print();
      return false;
   }

   try {
      (*ns)["_vistle"] = bp::import("_vistle");
   } catch (bp::error_already_set) {
      std::cerr << "vistle Python module import failed" << std::endl;
      PyErr_Print();
      return false;
   }

   if (!PythonEmbed::the().exec("import vistle"))
      return false;
   if (!PythonEmbed::the().exec("from vistle import *"))
      return false;

   return true;
}

PythonModule::PythonModule(int argc, char *argv[])
{
}


PythonModule::~PythonModule() {
}

} // namespace vistle
