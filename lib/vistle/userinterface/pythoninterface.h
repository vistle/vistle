#ifndef PYTHON_INTERFACE_H
#define PYTHON_INTERFACE_H

// include this before any Qt headers

#include <string>
#include <memory>
#include <vistle/util/pybind.h>

#include "export.h"

namespace vistle {

class V_UIEXPORT PythonInterface {
public:
    PythonInterface(const std::string &name);
    ~PythonInterface();
    static PythonInterface &the();
    pybind11::object &nameSpace();
    bool init();

    bool exec(const std::string &python);
    bool exec_file(const std::string &filename);

    static std::string errorString();

private:
    std::string m_name;
    std::unique_ptr<pybind11::object> m_namespace;
    static PythonInterface *s_singleton;
};

} // namespace vistle

#endif
