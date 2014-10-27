#ifndef PYTHON_MODULE_H
#define PYTHON_MODULE_H

#include <string>
#include <boost/python/object.hpp>

#include "export.h"

namespace vistle {

class VistleConnection;

class V_UIEXPORT PythonModule {

public:
   PythonModule(const std::string &path);
   PythonModule(VistleConnection *vc, const std::string &path);
   static PythonModule &the();

   VistleConnection &vistleConnection() const;
   bool import(boost::python::object *m_namespace, const std::string &path);

private:
   VistleConnection *m_vistleConnection;
   static PythonModule *s_instance;
};

} // namespace vistle

#endif
