#ifndef PYTHON_MODULE_H
#define PYTHON_MODULE_H

#include <string>
#include "export.h"
#include <vistle/util/buffer.h>
#include "pythonstateaccessor.h"

namespace pybind11 {
class object;
}

namespace vistle {

class ModuleCompound;
class StateTracker;

namespace message {
class Message;
}

class V_PYMODEXPORT PythonModule {
public:
    explicit PythonModule(PythonStateAccessor &stateAccessor);
    ~PythonModule();

    bool import(pybind11::object *m_namespace, const std::string &path);

    PythonStateAccessor *access();

private:
    PythonStateAccessor *m_access = nullptr;
};

void V_PYMODEXPORT moduleCompoundToFile(const ModuleCompound &comp);

} // namespace vistle

#endif
