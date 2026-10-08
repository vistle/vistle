#ifndef VISTLE_THRESHOLDVTKM_THRESHOLDVTKM_H
#define VISTLE_THRESHOLDVTKM_THRESHOLDVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class ThresholdVtkm: public vistle::VtkmModule {
public:
    ThresholdVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    vistle::IntParameter *m_invert = nullptr;
    vistle::IntParameter *m_operation = nullptr;
    vistle::FloatParameter *m_threshold = nullptr;

    std::unique_ptr<viskores::filter::Filter> setUpFilter(const InputData &input) const override;

    vistle::Object::const_ptr prepareOutputGrid(const InputData &input, OutputData &output) const override;

    vistle::DataBase::ptr prepareOutputField(const InputData &input, OutputData &output, int index,
                                             const std::string &fieldName) const override;
};

#endif
