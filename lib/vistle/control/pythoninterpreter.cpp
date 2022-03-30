#include <thread>
#include <mutex>

#include <vistle/python/pythoninterface.h>
#include <vistle/python/pythonmodule.h>
#include <vistle/control/hub.h>

#include "pythoninterpreter.h"

namespace vistle {

struct HubPythonStateAccessor: public vistle::PythonStateAccessor {
    void lock() override { state().getMutex().lock(); };
    void unlock() override { state().getMutex().unlock(); };
    vistle::StateTracker &state() override { return vistle::Hub::the().stateTracker(); }
    bool sendMessage(const vistle::message::Message &m, const vistle::buffer *payload = nullptr) override
    {
        bool ret = vistle::Hub::the().handleMessage(m, nullptr, payload);
        assert(ret);
        if (!ret) {
            std::cerr << "Python: failed to send message " << m << std::endl;
        }
        return ret;
    }
};

class Executor {
public:
    Executor(PythonInterpreter &inter, const std::string &filename, bool barrierAfterLoad, bool executeModules)
    : m_interpreter(inter)
    , m_filename(filename)
    , m_barrierAfterLoad(barrierAfterLoad)
    , m_executeModules(executeModules)
    , m_done(false)
    {
        if (m_executeModules)
            m_barrierAfterLoad = true;

        if (!m_interpreter.init())
            m_error = true;
        else
            m_py_release.reset(new pybind11::gil_scoped_release);
    }

    void operator()()
    {
        assert(!m_done);

        bool ok = !m_error;
        if (ok && !m_filename.empty()) {
            pybind11::gil_scoped_acquire acquire;
            ok = m_interpreter.executeFile(m_filename);
        }

        if (ok && m_barrierAfterLoad) {
            pybind11::gil_scoped_acquire acquire;
            ok = m_interpreter.executeCommand("barrier()");
            if (ok && m_executeModules)
                ok = m_interpreter.executeCommand("compute()");
        }

        std::lock_guard<std::mutex> locker(m_mutex);
        m_error = !ok;
        m_done = true;
    }

    bool done() const
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_done;
    }

    bool error() const { return m_error; }

private:
    mutable std::mutex m_mutex;
    PythonInterpreter &m_interpreter;
    const std::string &m_filename;
    bool m_barrierAfterLoad = false;
    bool m_executeModules = false;
    volatile bool m_done = false;
    volatile bool m_error = false;
    std::unique_ptr<pybind11::gil_scoped_release> m_py_release;
};

PythonInterpreter::PythonInterpreter(const std::string &file, const std::string &path, bool barrierAfterLoad,
                                     bool executeModules)
: m_pythonPath(path)
, m_interpreter(new PythonInterface("vistle"))
, m_access(new HubPythonStateAccessor)
, m_module(new PythonModule(*m_access))
, m_executor(new Executor(*this, file, barrierAfterLoad, executeModules))
, m_thread(std::ref(*m_executor))
{}

bool PythonInterpreter::init()
{
    if (!m_interpreter->init())
        return false;
    if (!m_module->import(&vistle::PythonInterface::the().nameSpace(), m_pythonPath))
        return false;
    return true;
}

bool PythonInterpreter::executeFile(const std::string &filename)
{
    return m_interpreter->exec_file(filename);
}

bool PythonInterpreter::executeCommand(const std::string &cmd)
{
    return m_interpreter->exec(cmd);
}

PythonInterpreter::~PythonInterpreter()
{
    m_thread.join();
    m_executor.reset();
    m_module.reset();
    m_interpreter.reset();
}

bool PythonInterpreter::check()
{
    if (m_executor->done()) {
        m_error = m_executor->error();
        return false;
    }

    return true;
}

bool PythonInterpreter::error() const
{
    return m_error;
}

} // namespace vistle
