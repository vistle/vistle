#ifndef VISTLE_CONTROL_PYTHONINTERPRETER_H
#define VISTLE_CONTROL_PYTHONINTERPRETER_H

#include <thread>
#include <mutex>

namespace pybind11 {
class gil_scoped_release;
}

namespace vistle {

class PythonInterface;
class PythonModule;
class PythonExecutor;
class Executor;

class PythonInterpreter {
public:
    PythonInterpreter(const std::string &path);
    ~PythonInterpreter();
    bool init();

    bool error() const;
    bool quitting() const;

    bool executeFile(const std::string &filename);
    bool executeCommand(const std::string &cmd);

private:
    std::string m_pythonPath;
    std::unique_ptr<pybind11::gil_scoped_release> m_py_release;
    std::shared_ptr<PythonInterface> m_interpreter;
    std::unique_ptr<PythonStateAccessor> m_access;
    std::shared_ptr<PythonModule> m_module;
    bool m_error = false;
};

class PythonExecutor {
public:
    enum State {
        Running,
        Success,
        Error,
        Wait,
    };

    enum Flags {
        Command = 0,
        LoadFile = 1,
        BarrierAfterLoad = 2,
        ExecuteModules = 4,
    };

    PythonExecutor(PythonInterpreter &inter, const std::string &command);
    PythonExecutor(PythonInterpreter &inter, int flags, const std::string &script);
    ~PythonExecutor();

    State state();
    bool done();

private:
    void run();

    PythonInterpreter &m_interpreter;
    std::string m_command;
    std::string m_script;
    int m_flags = Command;
    State m_state = Running;
    std::mutex m_mutex;
    std::thread m_thread;
};

} // namespace vistle
#endif
