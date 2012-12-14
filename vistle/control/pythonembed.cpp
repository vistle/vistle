#include <Python.h>
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>


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

   bp::class_<std::vector<int> >("PyVec")
      .def(bp::vector_indexing_suite<std::vector<int> >());

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
      //std::cerr << "Python exec error" << std::endl;
      PyErr_Print();
      PyErr_Clear();
      ok = false;
   } catch (...) {
      std::cerr << "Unknown Python exec error" << std::endl;
      ok = false;
   }

   return ok;
}

bool PythonEmbed::exec_file(const std::string &filename) {

   bool ok = false;
   try {
      bp::object r = bp::exec_file(filename.c_str(), m_namespace, m_namespace);
      if (r != bp::object()) {
         m_namespace["__result__"] = r;
         bp::exec("print __result__");
      }
      ok = true;
   } catch (bp::error_already_set) {
      //std::cerr << "Python exec error" << std::endl;
      PyErr_Print();
      PyErr_Clear();
      ok = false;
   } catch (...) {
      std::cerr << "Unknown Python exec error" << std::endl;
      ok = false;
   }

   return ok;
}

std::string PythonEmbed::raw_input(const std::string &prompt) {

   Communicator &comm = Communicator::the();
   int client = comm.currentClient();
   if (client >= 0) {
      comm.writeClient(client, prompt);
      std::string line = comm.readClientLine(client);
      char *end = &line[line.size()-1];
      if (end > &line[0] && *end == '\n') {
         --end;
         if (end > &line[0] && *end == '\r')
            --end;
      }

      return std::string(&line[0], end+1);
   }

   return std::string();
}

std::string PythonEmbed::readline() {

   Communicator &comm = Communicator::the();
   int client = comm.currentClient();
   if (client >= 0) {
      return comm.readClientLine(client);
   }

   return std::string();
}

void PythonEmbed::print_output(const std::string &str) {

   //std::cout << "OUT: " << str << std::endl;
   Communicator &comm = Communicator::the();
   int client = comm.currentClient();
   if (client >= 0) {
      comm.writeClient(client, str.c_str(), str.length());
   }
}

void PythonEmbed::print_error(const std::string &str) {

   //std::cerr << "ERR: " << str << std::endl;
   Communicator &comm = Communicator::the();
   int client = comm.currentClient();
   if (client >= 0) {
      comm.writeClient(client, str.c_str(), str.length());
   }
}

} // namespace vistle
