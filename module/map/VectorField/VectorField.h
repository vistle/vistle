#ifndef VISTLE_VECTORFIELD_VECTORFIELD_H
#define VISTLE_VECTORFIELD_VECTORFIELD_H

#include <vistle/module/module.h>

class VectorField: public vistle::Module {
public:
    VectorField(const std::string &name, int moduleID, mpi::communicator comm);
    ~VectorField();

private:
    virtual bool compute();

    vistle::FloatParameter *m_scale;
    vistle::VectorParameter *m_range;
    vistle::IntParameter *m_attachmentPoint;
    vistle::IntParameter *m_allCoordinates = nullptr;
};

#endif
