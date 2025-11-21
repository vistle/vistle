#ifndef VISTLE_TUBEVTKM_TUBEVTKM_H
#define VISTLE_TUBEVTKM_TUBEVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class TubeVtkm: public vistle::VtkmModule {
public:
    TubeVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    vistle::FloatParameter *m_radius;
    vistle::IntParameter *m_numberOfSides;
    vistle::IntParameter *m_addCaps;

    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;
};

#endif // VISTLE_TUBEVTKM_TUBEVTKM_H
