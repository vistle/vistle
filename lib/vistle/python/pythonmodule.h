#ifndef PYTHON_MODULE_H
#define PYTHON_MODULE_H

#include <string>
#include "export.h"
#include <vistle/util/buffer.h>

namespace pybind11 {
class object;
}

namespace vistle {

class ModuleCompound;
class StateTracker;

namespace message {
class Message;
}

struct V_PYEXPORT PythonStateAccessor {
    virtual ~PythonStateAccessor();
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual StateTracker &state() = 0;
    virtual bool sendMessage(const vistle::message::Message &m, const buffer *payload = nullptr) = 0;
};

class V_PYEXPORT PythonModule {
public:
    explicit PythonModule(PythonStateAccessor &stateAccessor);
    ~PythonModule();

    bool import(pybind11::object *m_namespace, const std::string &path);

    PythonStateAccessor *access();

private:
    PythonStateAccessor *m_access = nullptr;
};

void V_PYEXPORT moduleCompoundToFile(const ModuleCompound &comp);

} // namespace vistle

#endif
