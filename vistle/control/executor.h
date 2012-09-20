#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>

#include <scalar.h>
#include <vector.h>

namespace vistle {

class Communicator;

namespace message {
class Message;
};

class Executor {

   public:

      Executor(const std::string &name);
      ~Executor();

      virtual void config() = 0;

      void run();

      void spawn(const int moduleID, const char * name);
      void setParam(const int moduleID, const char * name, const vistle::Scalar value);
      void setParam(const int moduleID, const char * name, const std::string & value);
      void setParam(const int moduleID, const char * name, const vistle::Vector & value);
      void connect(const int moduleA, const char * aPort,
             const int moduleB, const char * bPort);
      void compute(const int moduleID);

   protected:
      bool handle(const message::Message &);

   private:
      std::string m_name;
      int m_rank, m_size;
      Communicator *m_comm;
};

} // namespace vistle

#endif
