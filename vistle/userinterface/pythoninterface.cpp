#include <Python.h>
#include <boost/python.hpp>

#include "pythoninterface.h"

namespace bp = boost::python;

namespace vistle {

PythonInterface *PythonInterface::s_singleton = NULL;

PythonInterface::PythonInterface(const std::string &name)
{
   assert(s_singleton == NULL);
   s_singleton = this;

#if PY_MAJOR_VERSION>2
   static std::wstring wideName = std::wstring(name.begin(), name.end());
   Py_SetProgramName((wchar_t *)wideName.c_str());
#else
   static std::string namebuf(name);
   Py_SetProgramName((char *)namebuf.c_str());
#endif
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

// cf. http://stackoverflow.com/questions/1418015/how-to-get-python-exception-text
// decode a Python exception into a string
std::string PythonInterface::errorString() {

    PyObject *exc,*val,*tb;
    PyErr_Fetch(&exc,&val,&tb);
    bp::handle<> hexc(exc), hval(bp::allow_null(val)), htb(bp::allow_null(tb));
    bp::object traceback(bp::import("traceback"));
    bp::object formatted_list;
    if (!tb) {
        bp::object format_exception_only(traceback.attr("format_exception_only"));
        formatted_list = format_exception_only(hexc, hval);
    } else {
        bp::object format_exception(traceback.attr("format_exception"));
        formatted_list = format_exception(hexc,hval,htb);
    }
    bp::object formatted = bp::str("\n").join(formatted_list);
    return bp::extract<std::string>(formatted);
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
