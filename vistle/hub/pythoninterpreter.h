#ifndef PYTHONINTERPRETER_H
#define PYTHONINTERPRETER_H

#include <thread>

namespace vistle {

class PythonInterface;
class PythonModule;
class Executor;

class PythonInterpreter {

 public:
   PythonInterpreter(const std::string &filename, const std::string &path);
   ~PythonInterpreter();

   bool check();

   bool executeFile(const std::string &filename);

 private:
   boost::shared_ptr<PythonInterface> m_interpreter;
   boost::shared_ptr<PythonModule> m_module;
   boost::shared_ptr<Executor> m_executor;
   std::thread m_thread;
};

} // namespace vistle

#endif
