#ifndef SPLITPOLYHEDRA_H
#define SPLITPOLYHEDRA_H

#include <vistle/module/module.h>

class SplitPolyhedra: public vistle::Module {
public:
    SplitPolyhedra(const std::string &name, int moduleID, mpi::communicator comm);
    ~SplitPolyhedra();

private:
    virtual bool compute();
};

#endif
