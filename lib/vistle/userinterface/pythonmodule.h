#ifndef PYTHON_MODULE_H
#define PYTHON_MODULE_H

#include <string>
#include "export.h"

namespace pybind11 {
class object;
}

namespace vistle {

class VistleConnection;
class ModuleCompound;
class V_UIEXPORT PythonModule {
public:
    explicit PythonModule(VistleConnection *vc = nullptr);
    ~PythonModule();

    VistleConnection &vistleConnection() const;
    bool import(pybind11::object *m_namespace, const std::string &path);

private:
    VistleConnection *m_vistleConnection;
};
void V_UIEXPORT moduleCompoundToFile(const ModuleCompound &comp);

} // namespace vistle

#endif
