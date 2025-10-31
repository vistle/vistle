#ifndef VISTLE_SPHERESOVERLAP_SPHERESOVERLAP_H
#define VISTLE_SPHERESOVERLAP_SPHERESOVERLAP_H

#include <string>

#include <vistle/module/module.h>

/*
    Detects which spheres overlap and creates a line between them.
*/
class SpheresOverlap: public vistle::Module {
public:
    SpheresOverlap(const std::string &name, int moduleID, mpi::communicator comm);

private:
    vistle::Port *m_spheresIn, *m_linesOut, *m_dataOut;
    vistle::FloatParameter *m_radiusCoefficient;
    vistle::IntParameter *m_thicknessDeterminer;

    // TODO: delete this
    vistle::IntParameter *m_useVtkm;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif
