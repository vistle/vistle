#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>

#include <vistle/core/scalar.h>
#include <vistle/core/paramvector.h>

namespace vistle {

class Communicator;
class Module;

namespace message {
class Message;
}

class Executor {
public:
    Executor(int argc, char *argv[], boost::mpi::communicator comm);
    virtual ~Executor();

    virtual bool config(int argc, char *argv[]);

    void run();

    void setVistleRoot(const std::string &directory);

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
