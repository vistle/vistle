#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <userinterface/pythoninterface.h>
#include <userinterface/pythonmodule.h>

#include "pythoninterpreter.h"

namespace vistle {

class Executor {

 public:
   Executor(PythonInterpreter &inter, const std::string &filename)
   : m_done(false)
   , m_interpreter(inter)
   , m_filename(filename)
   {
   }

   void operator()() {

      if (!m_filename.empty())
         m_interpreter.executeFile(m_filename);

      boost::lock_guard<boost::mutex> locker(m_mutex);
      m_done = true;
   }

   bool done() const {

      boost::lock_guard<boost::mutex> locker(m_mutex);
      return m_done;
   }

 private:
   mutable boost::mutex m_mutex;
   bool m_done;
   PythonInterpreter &m_interpreter;
   const std::string &m_filename;
};

PythonInterpreter::PythonInterpreter(const std::string &file, const std::string &path)
: m_interpreter(new PythonInterface("vistle"))
, m_module(new PythonModule(path))
, m_executor(new Executor(*this, file))
, m_thread(boost::ref(*m_executor))
{
}

bool PythonInterpreter::executeFile(const std::string &filename) {

   return m_interpreter->exec_file(filename);
}

PythonInterpreter::~PythonInterpreter() {

}

bool PythonInterpreter::check() {

   return !m_executor->done();
}

} // namespace vistle
