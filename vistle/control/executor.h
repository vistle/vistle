#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>

#include <core/scalar.h>
#include <core/paramvector.h>

#include "export.h"

namespace vistle {

class Communicator;
class PythonEmbed;

namespace message {
class Message;
};

class V_CONTROLEXPORT Executor {

   public:

      Executor(int argc, char *argv[]);
      virtual ~Executor();

      virtual bool config(int argc, char *argv[]);

      void run();

      void setInput(const std::string &input);
      void setFile(const std::string &filename);

      int getRank() const { return m_rank; }
      int getSize() const { return m_size; }

      unsigned short uiPort() const;

   private:
      std::string m_name;
      int m_rank, m_size;

      Communicator *m_comm;
      PythonEmbed *m_interpreter;

      int m_argc;
      char **m_argv;
};

} // namespace vistle

#endif
