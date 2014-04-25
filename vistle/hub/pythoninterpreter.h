#ifndef PYTHONINTERPRETER_H
#define PYTHONINTERPRETER_H

#include <boost/thread.hpp>

namespace vistle {

class PythonInterface;
class PythonModule;
class Executor;

class PythonInterpreter {

 public:
   PythonInterpreter(const std::string &filename);
   ~PythonInterpreter();

   bool check();

   bool executeFile(const std::string &filename);

 private:
   boost::shared_ptr<PythonInterface> m_interpreter;
   boost::shared_ptr<PythonModule> m_module;
   boost::shared_ptr<Executor> m_executor;
   boost::thread m_thread;
};

} // namespace vistle

#endif
