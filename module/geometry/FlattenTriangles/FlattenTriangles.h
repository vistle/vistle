#ifndef FLATTENTRIANGLES_H
#define FLATTENTRIANGLES_H

#include <vistle/module/module.h>

class FlattenTriangles: public vistle::Module {
public:
    FlattenTriangles(const std::string &name, int moduleID, mpi::communicator comm);
    ~FlattenTriangles();

private:
    virtual bool compute();
};

#endif
