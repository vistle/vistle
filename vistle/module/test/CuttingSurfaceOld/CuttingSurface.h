#ifndef CUTTINGSURFACE_H
#define CUTTINGSURFACE_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>

class CuttingSurfaceOld: public vistle::Module {
public:
    CuttingSurfaceOld(const std::string &name, int moduleID, mpi::communicator comm);
    ~CuttingSurfaceOld();

private:
    std::pair<vistle::Object::ptr, vistle::Object::ptr> generateCuttingSurface(vistle::Object::const_ptr grid,
                                                                               vistle::Object::const_ptr data,
                                                                               const vistle::Vector &normal,
                                                                               const vistle::Scalar distance);

    virtual bool compute();
};

#endif
