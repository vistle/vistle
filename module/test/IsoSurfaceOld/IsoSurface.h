#ifndef ISOSURFACE_H
#define ISOSURFACE_H

#include <vistle/module/module.h>

class IsoSurfaceOld: public vistle::Module {
public:
    IsoSurfaceOld(const std::string &name, int moduleID, mpi::communicator comm);
    ~IsoSurfaceOld();

private:
    vistle::Object::ptr generateIsoSurface(vistle::Object::const_ptr grid, vistle::Object::const_ptr data,
                                           const vistle::Scalar isoValue);

    virtual bool compute();
    virtual bool prepare();
    virtual bool reduce(int timestep);

    vistle::FloatParameter *m_isovalue;

    vistle::Scalar min, max;
};

#endif
