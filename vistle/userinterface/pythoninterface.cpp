#include <Python.h>
#include <boost/python.hpp>
//#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include "pythoninterface.h"

namespace bp = boost::python;

namespace vistle {

PythonInterface *PythonInterface::s_singleton = NULL;

PythonInterface::PythonInterface(const std::string &name)
{
   assert(s_singleton == NULL);
   s_singleton = this;

   static char namebuf[1024];
   strncpy(namebuf, name.c_str(), sizeof(namebuf));
   namebuf[sizeof(namebuf)-1] = '\0';
   Py_SetProgramName(namebuf);
   Py_Initialize();

   bp::object main = bp::import("__main__");
   m_namespace = main.attr("__dict__");
}


PythonInterface::~PythonInterface() {

   Py_Finalize();
}

PythonInterface &PythonInterface::the() {

   assert(s_singleton);
   return *s_singleton;
}

boost::python::object &PythonInterface::nameSpace()
{
   return m_namespace;
}

bool PythonInterface::exec(const std::string &python) {

   return exec_wrapper(python, bp::exec);
}

bool PythonInterface::exec_file(const std::string &filename) {

   return exec_wrapper(filename, bp::exec_file);
}

bool PythonInterface::exec_wrapper(const std::string &param, PythonInterface::execfunc func)
{
   bool ok = false;
   try {
      bp::object r = func(param.c_str(), m_namespace, m_namespace);
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
   } catch (std::exception &ex) {
      std::cerr << "Unknown Python exec error: " << ex.what() << std::endl;
      ok = false;
   } catch (...) {
      std::cerr << "Unknown Python exec error" << std::endl;
      ok = false;
   }

   return ok;

}

} // namespace vistle
