#ifndef THICKEN_H
#define THICKEN_H

#include <vistle/module/module.h>

class Thicken: public vistle::Module {
public:
    Thicken(const std::string &name, int moduleID, mpi::communicator comm);
    ~Thicken();

private:
    virtual bool compute();

    vistle::FloatParameter *m_radius;
    vistle::FloatParameter *m_sphereScale;
    vistle::IntParameter *m_mapMode;
    vistle::VectorParameter *m_range;
    vistle::IntParameter *m_startStyle, *m_jointStyle, *m_endStyle;

    enum OutputGeometry { OGUnknown, OGSpheres, OGTubes, OGError };
    OutputGeometry m_output = OGUnknown;
};

#endif
