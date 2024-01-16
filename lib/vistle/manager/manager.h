#ifndef VISTLE_MANAGER_LIB_H
#define VISTLE_MANAGER_LIB_H

#include "export.h"
#include <string>
#include <memory>
#include <mutex>

#include <boost/mpi.hpp>


namespace vistle {

class Executor;
class Module;

class V_MANAGEREXPORT VistleManager {
public:
    static bool inPlugin();
    //if not called from vistle, this assumes MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided); is set
    bool run(int argc, char *argv[]);
    ~VistleManager();

private:
    boost::mpi::communicator m_comm;
    Executor *executer = nullptr;
};
} // namespace vistle


#endif // !VISTLE_MANAGER_LIB_H
