#ifndef VISTLE_VECTORFIELDVTKM_VECTORFIELDVTKM_H
#define VISTLE_VECTORFIELDVTKM_VECTORFIELDVTKM_H

#include <vistle/vtkm/vtkm_module.h>


class VectorFieldVtkm: public vistle::VtkmModule {
public:
    VectorFieldVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;

    vistle::FloatParameter *m_scale = nullptr;
    vistle::VectorParameter *m_range = nullptr;
    vistle::IntParameter *m_attachment = nullptr;
};

#endif // VISTLE_VECTORFIELDVTKM_VECTORFIELDVTKM_H
