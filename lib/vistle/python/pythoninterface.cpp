#include <cstdio>
#include <cassert>
#include <iostream>

#include <pybind11/embed.h>
#include <vistle/util/pybind.h>

#include "pythoninterface.h"

namespace py = pybind11;

namespace vistle {

PythonInterface *PythonInterface::s_singleton = NULL;

PythonInterface::PythonInterface(const std::string &name): m_name(name)
{
    assert(s_singleton == nullptr);
    s_singleton = this;
}


PythonInterface::~PythonInterface()
{
    assert(s_singleton == this);
    s_singleton = nullptr;

    m_namespace.reset();
    py::finalize_interpreter();
}

PythonInterface &PythonInterface::the()
{
    assert(s_singleton);
    return *s_singleton;
}

py::object &PythonInterface::nameSpace()
{
    if (!m_namespace)
        m_namespace.reset(new py::object);
    return *m_namespace;
}

bool PythonInterface::init()
{
    py::initialize_interpreter();
    py::object mainmod = py::module::import("__main__");
    m_namespace.reset(new py::object);
    *m_namespace = mainmod.attr("__dict__");

#ifdef PY_MAJOR_VERSION
#if PY_MAJOR_VERSION > 2
    static std::wstring wideName = std::wstring(m_name.begin(), m_name.end());
    Py_SetProgramName(const_cast<wchar_t *>(wideName.c_str()));
#else
    Py_SetProgramName(const_cast<char *>(m_name.c_str()));
#endif
#endif

    return true;
}

// cf. http://stackoverflow.com/questions/1418015/how-to-get-python-exception-text
// decode a Python exception into a string
std::string PythonInterface::errorString()
{
#if 0
    PyObject *exc,*val,*tb;
    PyErr_Fetch(&exc,&val,&tb);
    py::handle hexc(exc), hval(py::allow_null(val)), htb(py::allow_null(tb));
    py::module traceback(py::module::import("traceback"));
    py::object formatted_list;
    if (!tb) {
        bp::object format_exception_only(traceback.attr("format_exception_only"));
        formatted_list = format_exception_only(hexc, hval);
    } else {
        bp::object format_exception(traceback.attr("format_exception"));
        formatted_list = format_exception(hexc,hval,htb);
    }
    py::object formatted = py::str("\n").join(formatted_list);
    return formatted.cast<std::string>();
#endif
    PyErr_Print();
    PyErr_Clear();
    return "Python ERROR";
}

bool PythonInterface::exec(const std::string &python)
{
    bool ok = false;
    try {
        py::object r = py::eval<py::eval_statements>(python.c_str(), py::globals(), nameSpace());
        if (r.ptr() && !r.is_none()) {
            py::print(r, '\n');
        }
        ok = true;
    } catch (py::error_already_set &ex) {
        std::cerr << "PythonInterface::exec(" << python << "): Error: " << ex.what() << std::endl;
        //std::cerr << "Python exec error" << std::endl;
        PyErr_Print();
        PyErr_Clear();
        ok = false;
    } catch (std::exception &ex) {
        std::cerr << "PythonInterface::exec(" << python << "): Unknown error: " << ex.what() << std::endl;
        ok = false;
    } catch (...) {
        std::cerr << "PythonInterface::exec(" << python << "): Unknown error" << std::endl;
        ok = false;
    }
    return ok;
}

bool PythonInterface::exec_file(const std::string &filename)
{
    bool ok = false;
    try {
        py::object r = py::eval_file<py::eval_statements>(filename.c_str(), py::globals(), nameSpace());
        if (r.ptr()) {
            py::print(r);
            py::print("\n");
        }
        ok = true;
    } catch (py::error_already_set &ex) {
        std::cerr << "PythonInterface::exec_file(" << filename << "): Error: " << ex.what() << std::endl;
        //std::cerr << "Python exec error" << std::endl;
        PyErr_Print();
        PyErr_Clear();
        ok = false;
    } catch (std::exception &ex) {
        std::cerr << "PythonInterface::exec_file(" << filename << "): Unknown error: " << ex.what() << std::endl;
        ok = false;
    } catch (...) {
        std::cerr << "PythonInterface::exec_file(" << filename << "): Unknown error" << std::endl;
        ok = false;
    }

    return ok;
}

} // namespace vistle
