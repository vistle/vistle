#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>

#include <core/scalar.h>
#include <core/paramvector.h>

namespace vistle {

class Communicator;

namespace message {
class Message;
}

class Executor {

   public:

      Executor(int argc, char *argv[]);
      virtual ~Executor();

      virtual bool config(int argc, char *argv[]);

      void run();

      void setModuleDir(const std::string &directory);

      int getRank() const { return m_rank; }
      int getSize() const { return m_size; }

   private:
      std::string m_name;
      int m_rank, m_size;

      Communicator *m_comm;

      int m_argc;
      char **m_argv;

      Executor(const Executor &other); // not implemented
};

} // namespace vistle

#endif
