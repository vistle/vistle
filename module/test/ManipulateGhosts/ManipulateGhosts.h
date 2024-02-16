#ifndef MANIPULATEGHOSTS_H
#define MANIPULATEGHOSTS_H

#include <vistle/module/module.h>

class ManipulateGhosts: public vistle::Module {
public:
    ManipulateGhosts(const std::string &name, int moduleID, mpi::communicator comm);
    ~ManipulateGhosts();

private:
    vistle::Port *m_gridIn = nullptr, *m_gridOut = nullptr;
    vistle::IntParameter *m_operation = nullptr;

    virtual bool compute();
};

#endif
