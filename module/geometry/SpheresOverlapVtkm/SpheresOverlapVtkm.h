#ifndef VISTLE_SPHERESOVERLAPVTKM_SPHERESOVERLAPVTKM_H
#define VISTLE_SPHERESOVERLAPVTKM_SPHERESOVERLAPVTKM_H

#include <string>

#include <vistle/module/module.h>

/*
    Detects which spheres overlap and creates a line between them.
*/
class SpheresOverlapVtkm: public vistle::Module {
public:
    SpheresOverlapVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    vistle::Port *m_spheresIn, *m_linesOut, *m_dataOut;
    vistle::FloatParameter *m_radiusCoefficient;
    vistle::IntParameter *m_thicknessDeterminer;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    bool checkAndNotify(const ModuleStatusPtr &status) const;
};

#endif
