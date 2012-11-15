#ifndef PYTHON_MODULE_H
#define PYTHON_MODULE_H

#include <string>
#include <boost/python/object.hpp>

namespace vistle {

class PythonModule {
   public:

      PythonModule(int argc, char *argv[]);
      ~PythonModule();

      static bool import(boost::python::object *m_namespace);
};

} // namespace vistle

#endif
