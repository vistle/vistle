#include <Python.h>
#include <boost/python.hpp>


#include <core/message.h>
#include "communicator.h"
#include "pythonembed.h"
#include "pythonmodule.h"

namespace bp = boost::python;

namespace vistle {

PythonEmbed *PythonEmbed::s_singleton = NULL;

PythonEmbed::PythonEmbed(int argc, char *argv[])
{
   assert(s_singleton == NULL);
   s_singleton = this;

   Py_SetProgramName(argv[0]);
   Py_Initialize();

   bp::object main = bp::import("__main__");
   m_namespace = main.attr("__dict__");

   PythonModule::import(&m_namespace);
}


PythonEmbed::~PythonEmbed() {

    Py_Finalize();
}

PythonEmbed &PythonEmbed::the() {

   assert(s_singleton);
   return *s_singleton;
}

bool PythonEmbed::exec(const std::string &python) {

   bool ok = false;
   try {
      bp::object r = bp::exec(python.c_str(), m_namespace, m_namespace);
      if (r != bp::object()) {
         m_namespace["__result__"] = r;
         bp::exec("print __result__");
      }
      ok = true;
   } catch (bp::error_already_set) {
      std::cerr << "Python exec error" << std::endl;
      PyErr_Print();
      PyErr_Clear();
      ok = false;
   } catch (...) {
      std::cerr << "Unknown Python exec error" << std::endl;
      ok = false;
   }

   return ok;
}

void PythonEmbed::print_output(const char *str) {

   std::cout << "OUT: " << str << std::endl;
   Communicator &comm = Communicator::the();
   int client = comm.currentClient();
   if (client >= 0) {
      comm.writeClient(client, str, strlen(str));
   }
}

void PythonEmbed::print_error(const char *str) {

   std::cerr << "ERR: " << str << std::endl;
   Communicator &comm = Communicator::the();
   int client = comm.currentClient();
   if (client >= 0) {
      comm.writeClient(client, str, strlen(str));
   }
}

} // namespace vistle
