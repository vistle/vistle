#ifndef TOTRIANGLES_H
#define TOTRIANGLES_H

#include <vistle/module/module.h>

class ToTriangles: public vistle::Module {
public:
    ToTriangles(const std::string &name, int moduleID, mpi::communicator comm);
    ~ToTriangles();

private:
    virtual bool compute();

    vistle::IntParameter *p_transformSpheres = nullptr;
};

#endif
