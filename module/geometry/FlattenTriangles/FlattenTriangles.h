#ifndef VISTLE_FLATTENTRIANGLES_FLATTENTRIANGLES_H
#define VISTLE_FLATTENTRIANGLES_FLATTENTRIANGLES_H

#include <vistle/module/module.h>

class FlattenTriangles: public vistle::Module {
public:
    FlattenTriangles(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool compute() override;
};

#endif
