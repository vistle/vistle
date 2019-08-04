#include <thread>
#include <mutex>

#include <userinterface/pythoninterface.h>
#include <userinterface/pythonmodule.h>

#include "pythoninterpreter.h"

namespace vistle {

class Executor {

 public:
   Executor(PythonInterpreter &inter, const std::string &filename, bool executeModules)
   : m_interpreter(inter)
   , m_filename(filename)
   , m_executeModules(executeModules)
   , m_done(false)
   {
       m_interpreter.init();
   }

   void operator()() {

      if (!m_filename.empty())
         m_interpreter.executeFile(m_filename);

      if (m_executeModules) {
          m_interpreter.executeCommand("barrier()");
          m_interpreter.executeCommand("compute()");
      }

      std::lock_guard<std::mutex> locker(m_mutex);
      m_done = true;
   }

   bool done() const {

      std::lock_guard<std::mutex> locker(m_mutex);
      return m_done;
   }

 private:
   mutable std::mutex m_mutex;
   PythonInterpreter &m_interpreter;
   const std::string &m_filename;
   bool m_executeModules = false;
   bool m_done;
};

PythonInterpreter::PythonInterpreter(const std::string &file, const std::string &path, bool executeModules)
: m_pythonPath(path)
, m_interpreter(new PythonInterface("vistle"))
, m_module(new PythonModule)
, m_executor(new Executor(*this, file, executeModules))
, m_thread(std::ref(*m_executor))
{
}

void PythonInterpreter::init() {
   m_interpreter->init();
   m_module->import(&vistle::PythonInterface::the().nameSpace(), m_pythonPath);
}

bool PythonInterpreter::executeFile(const std::string &filename) {

    try {
        return m_interpreter->exec_file(filename);
    } catch (...) {
        std::cerr << "executing Python file " << filename << " failed" << std::endl;
    }
    return false;
}

bool PythonInterpreter::executeCommand(const std::string &cmd)
{
    try {
        return m_interpreter->exec(cmd);
    } catch (...) {
        std::cerr << "executing Python command " << cmd << " failed" << std::endl;
    }
    return false;
}

PythonInterpreter::~PythonInterpreter() {

   m_thread.join();
   m_executor.reset();
   m_module.reset();
   m_interpreter.reset();
}

bool PythonInterpreter::check() {

   return !m_executor->done();
}

} // namespace vistle
