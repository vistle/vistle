#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>

#include <core/scalar.h>
#include <core/vector.h>

namespace vistle {

class Communicator;
class PythonEmbed;

namespace message {
struct Message;
};

class Executor {

   public:

      Executor(int argc, char *argv[]);
      virtual ~Executor();

      virtual void config() = 0;

      void run();

      void registerInterpreter(PythonEmbed *pi);

      int getRank() const { return m_rank; }
      int getSize() const { return m_size; }

   private:
      std::string m_name;
      int m_rank, m_size;
      Communicator *m_comm;
};

} // namespace vistle

#endif
