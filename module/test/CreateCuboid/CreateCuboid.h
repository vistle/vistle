#ifndef CREATECUBOID_H
#define CREATECUBOID_H

#include <vistle/module/module.h>

class CreateCuboid: public vistle::Module {
public:
    CreateCuboid(const std::string &name, int moduleID, mpi::communicator comm);
    ~CreateCuboid();

private:
    virtual bool compute();
};

#endif // CREATECUBOID_H
