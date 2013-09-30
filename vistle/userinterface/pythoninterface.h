#ifndef PYTHON_INTERFACE_H
#define PYTHON_INTERFACE_H

#include <string>
#include <iostream>
#include <boost/python/object.hpp>
#include <boost/python/exec.hpp>

#include "export.h"

namespace vistle {

class V_UIEXPORT PythonInterface {
   public:

      PythonInterface(const std::string &name);
      ~PythonInterface();
      static PythonInterface &the();
      boost::python::object &nameSpace();

      bool exec(const std::string &python);
      bool exec_file(const std::string &filename);

   private:
      boost::python::object m_namespace;
      static PythonInterface *s_singleton;

      typedef boost::python::object (*execfunc)(boost::python::str expression,
            boost::python::object globals,
            boost::python::object locals);

      bool exec_wrapper(const std::string &param, execfunc func);
};

} // namespace vistle

#endif
