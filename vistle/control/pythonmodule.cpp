#include <Python.h>
#include <boost/python.hpp>


#include <core/message.h>
#include "communicator.h"
#include "pythonmodule.h"
#include "pythonembed.h"

namespace bp = boost::python;

namespace vistle {

static void print_output(const char *str) {

   PythonEmbed::print_output(str);
}

static void print_error(const char *str) {

   PythonEmbed::print_error(str);
}

static void quit() {

   std::cerr << "Python: quit" << std::endl;
   message::Quit m(0, Communicator::the().getRank());
   Communicator::the().broadcastAndHandleMessage(m);
   Communicator::the().setQuitFlag();
}

static void debug(char c) {

   std::cerr << "Python: debug: " << c << std::endl;
   message::Debug m(0, Communicator::the().getRank(), c);
   Communicator::the().broadcastAndHandleMessage(m);
}

static int spawn(const char *module) {
   std::cerr << "Python: spawn "<< module << std::endl;
   int id = Communicator::the().newModuleID();
   message::Spawn m(0, Communicator::the().getRank(), id, module);
   Communicator::the().broadcastAndHandleMessage(m);
   return id;
}

static void kill(int id) {
   std::cerr << "Python: kill "<< id << std::endl;
   message::Kill m(0, Communicator::the().getRank(), id);
   Communicator::the().broadcastAndHandleMessage(m);
}

static void connect(int sid, const char *sport, int did, const char *dport) {

   std::cerr << "Python: connect "<< sid << ":" << sport << " -> " << did << ":" << dport << std::endl;
   message::Connect m(0, Communicator::the().getRank(),
         sid, sport, did, dport);
   Communicator::the().broadcastAndHandleMessage(m);
}

static void setIntParam(int id, const char *name, int value) {

   std::cerr << "Python: setIntParam " << id << ":" << name << " = " << value << std::endl;
   message::SetIntParameter m(0, Communicator::the().getRank(),
         id, name, value);
   Communicator::the().broadcastAndHandleMessage(m);
}

static void setFloatParam(int id, const char *name, double value) {

   std::cerr << "Python: setFloatParam " << id << ":" << name << " = " << value << std::endl;
   message::SetFloatParameter m(0, Communicator::the().getRank(),
         id, name, value);
   Communicator::the().broadcastAndHandleMessage(m);
}

static void setVectorParam(int id, const char *name, double v1, double v2, double v3) {

   std::cerr << "Python: setVectorParam " << id << ":" << name << " = " << v1 << " " << v2 << " " << v3 << std::endl;
   message::SetVectorParameter m(0, Communicator::the().getRank(),
         id, name, Vector(v1, v2, v3));
   Communicator::the().broadcastAndHandleMessage(m);
}

static void setFileParam(int id, const char *name, const std::string &value) {

   std::cerr << "Python: setFileParam " << id << ":" << name << " = " << value << std::endl;
   message::SetFileParameter m(0, Communicator::the().getRank(),
         id, name, value);
   Communicator::the().broadcastAndHandleMessage(m);
}

static void compute(int id) {

   std::cerr << "Python: compute " << id << std::endl;
   message::Compute m(0, Communicator::the().getRank(), id);
   Communicator::the().broadcastAndHandleMessage(m);
}

#define param(T, f) \
   def("set" #T "Param", f, "set parameter `arg2` of module with ID `arg1` to `arg3`"); \
   def("setParam", f, "set parameter `arg2` of module with ID `arg1` to `arg3`");

BOOST_PYTHON_MODULE(_vistle)
{
    using namespace boost::python;
    def("_print_error", print_error);
    def("_print_output", print_output);

    def("spawn", spawn, "spawn new module `arg1`\n" "return its ID");
    def("kill", kill, "kill module with ID `arg1`");
    def("connect", connect, "connect output `arg2` of module with ID `arg1` to input `arg4` of module with ID `arg3`");
    def("compute", compute, "trigger execution of module with IT `arg1`");
    def("quit", quit, "quit vistle session");
    def("debug", debug, "send first character of `arg1` to every vistle process");

    param(Int, setIntParam);
    param(Float, setFloatParam);
    param(File, setFileParam);
    param(Vector, setVectorParam);
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
