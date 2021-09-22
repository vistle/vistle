#ifndef PYTHONINTERPRETER_H
#define PYTHONINTERPRETER_H

#include <thread>

namespace vistle {

class PythonInterface;
class PythonModule;
class Executor;

class PythonInterpreter {
public:
    PythonInterpreter(const std::string &filename, const std::string &path, bool executeModules = false);
    ~PythonInterpreter();
    bool init();

    bool check();
    bool error() const;

    bool executeFile(const std::string &filename);
    bool executeCommand(const std::string &cmd);

private:
    std::string m_pythonPath;
    std::shared_ptr<PythonInterface> m_interpreter;
    std::shared_ptr<PythonModule> m_module;
    std::shared_ptr<Executor> m_executor;
    std::thread m_thread;
    bool m_error = false;
};

} // namespace vistle

#endif
