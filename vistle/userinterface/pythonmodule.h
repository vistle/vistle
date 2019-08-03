#ifndef PYTHON_MODULE_H
#define PYTHON_MODULE_H

#include <string>
#include "export.h"

namespace pybind11 {
class object;
}

namespace vistle {

class VistleConnection;

class V_UIEXPORT PythonModule {

public:
   explicit PythonModule(VistleConnection *vc = nullptr);
   ~PythonModule();
   static PythonModule &the();

   VistleConnection &vistleConnection() const;
   bool import(pybind11::object *m_namespace, const std::string &path);

private:
   VistleConnection *m_vistleConnection;
   static PythonModule *s_instance;
};

} // namespace vistle

#endif
