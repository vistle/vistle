#ifndef VISTLE_THRESHOLDVTKM_THRESHOLDVTKM_H
#define VISTLE_THRESHOLDVTKM_THRESHOLDVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class ThresholdVtkm: public VtkmModule {
public:
    ThresholdVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~ThresholdVtkm();

private:
    std::unique_ptr<vtkm::filter::Filter> setUpFilter() const override;

    vistle::IntParameter *m_invert = nullptr;
    vistle::IntParameter *m_operation = nullptr;
    vistle::FloatParameter *m_threshold = nullptr;
};

#endif
