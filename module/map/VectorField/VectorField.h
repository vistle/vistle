#ifndef VISTLE_VECTORFIELD_VECTORFIELD_H
#define VISTLE_VECTORFIELD_VECTORFIELD_H

#include <vistle/module/module.h>

class VectorField: public vistle::Module {
public:
    VectorField(const std::string &name, int moduleID, mpi::communicator comm);
    ~VectorField();

private:
    virtual bool compute();

    vistle::Port *m_gridIn = nullptr, *m_dataIn = nullptr;
    vistle::Port *m_gridOut = nullptr, *m_dataOut = nullptr;

    vistle::FloatParameter *m_scale = nullptr;
    vistle::VectorParameter *m_range = nullptr;
    vistle::IntParameter *m_attachmentPoint = nullptr;
    vistle::IntParameter *m_allCoordinates = nullptr;
};

#endif
