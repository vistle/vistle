#include <thread>
#include <mutex>

#include <vistle/python/pythoninterface.h>
#include <vistle/python/pythonmodule.h>
#include <vistle/control/hub.h>
#include <vistle/util/threadname.h>

#include "pythoninterpreter.h"

namespace vistle {

struct HubPythonStateAccessor: public vistle::PythonStateAccessor {
    void lock() override { state().getMutex().lock(); }
    void unlock() override { state().getMutex().unlock(); }
    vistle::StateTracker &state() override { return vistle::Hub::the().stateTracker(); }
    bool sendMessage(const vistle::message::Message &m, const vistle::buffer *payload = nullptr) override
    {
        bool ret = vistle::Hub::the().handleMessage(m, nullptr, payload, vistle::message::Identify::SCRIPT);
        assert(ret);
        if (!ret) {
            std::cerr << "Python: failed to send message " << m << std::endl;
        }
        return ret;
    }
};

PythonInterpreter::PythonInterpreter(const std::string &path)
: m_pythonPath(path)
, m_interpreter(new PythonInterface("vistle"))
, m_access(new HubPythonStateAccessor)
, m_module(new PythonModule(*m_access))
{
    m_error = !init();
    if (!m_error) {
        m_py_release.reset(new pybind11::gil_scoped_release);
    }
}

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
    if (m_error)
        return false;
    return m_interpreter->exec_file(filename);
}

bool PythonInterpreter::executeCommand(const std::string &cmd)
{
    if (m_error)
        return false;
    return m_interpreter->exec(cmd);
}

PythonInterpreter::~PythonInterpreter()
{
    m_py_release.reset();
    m_module.reset();
    m_interpreter.reset();
}

bool PythonInterpreter::error() const
{
    return m_error;
}

bool PythonInterpreter::quitting() const
{
    std::lock_guard stateLocker(*m_access);
    return m_access->state().quitting();
}

PythonExecutor::PythonExecutor(PythonInterpreter &inter, const std::string &command)
: m_interpreter(inter), m_command(command), m_thread([this]() { run(); })
{}

PythonExecutor::PythonExecutor(PythonInterpreter &inter, int flags, const std::string &script)
: m_interpreter(inter), m_script(script), m_flags(flags), m_thread([this]() { run(); })
{}

PythonExecutor::~PythonExecutor()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    m_thread.join();
}

void PythonExecutor::run()
{
    setThreadName("Python:" + m_script);
    assert(m_state == Running);

    bool ok = true;
    if (ok) {
        pybind11::gil_scoped_acquire acquire;
        if (ok && !m_script.empty()) {
            ok = m_interpreter.executeFile(m_script);
        }
        if (ok && !m_command.empty()) {
            ok = m_interpreter.executeCommand(m_command);
        }
        if (ok && (m_flags & BarrierAfterLoad)) {
            if (!m_interpreter.quitting()) {
                ok = m_interpreter.executeCommand("barrier(\"after load, automatic\")");
            }
        }
        if (ok && (m_flags & ExecuteModules)) {
            if (!m_interpreter.quitting()) {
                ok = m_interpreter.executeCommand("compute()");
            }
        }
    }

    std::lock_guard<std::mutex> locker(m_mutex);
    m_state = ok ? Success : Error;
}

bool PythonExecutor::done()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_state == Error || m_state == Success;
}

PythonExecutor::State PythonExecutor::state()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_state;
}

} // namespace vistle
