#ifndef PYTHONINTERPRETER_H
#define PYTHONINTERPRETER_H

#include <thread>

namespace vistle {

class PythonInterface;
class PythonModule;
class Executor;

class PythonInterpreter {
public:
    enum Flags {
        Command = 0,
        LoadFile = 1,
        BarrierAfterLoad = 2,
        ExecuteModules = 4,
    };
    PythonInterpreter(const std::string &str, const std::string &path, int flags);
    ~PythonInterpreter();
    bool init();

    bool check();
    bool error() const;

    bool executeFile(const std::string &filename);
    bool executeCommand(const std::string &cmd);

private:
    std::string m_pythonPath;
    std::shared_ptr<PythonInterface> m_interpreter;
    std::unique_ptr<PythonStateAccessor> m_access;
    std::shared_ptr<PythonModule> m_module;
    std::shared_ptr<Executor> m_executor;
    std::thread m_thread;
    bool m_error = false;
};

} // namespace vistle

#endif
