#ifndef VISTLE_THICKEN_THICKEN_H
#define VISTLE_THICKEN_THICKEN_H

#include <vistle/module/module.h>

class Thicken: public vistle::Module {
public:
    Thicken(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool compute() override;

    vistle::FloatParameter *m_radius;
    vistle::FloatParameter *m_sphereScale;
    vistle::IntParameter *m_mapMode;
    vistle::VectorParameter *m_range;
    vistle::IntParameter *m_startStyle, *m_jointStyle, *m_endStyle;
    vistle::IntParameter *m_correctDepth;

    enum OutputGeometry { OGUnknown, OGSpheres, OGTubes, OGError };
    OutputGeometry m_output = OGUnknown;
};

#endif
