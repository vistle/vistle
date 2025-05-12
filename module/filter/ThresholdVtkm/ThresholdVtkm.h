#ifndef VISTLE_THRESHOLDVTKM_THRESHOLDVTKM_H
#define VISTLE_THRESHOLDVTKM_THRESHOLDVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class ThresholdVtkm: public vistle::VtkmModule {
public:
    ThresholdVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~ThresholdVtkm();

private:
    vistle::IntParameter *m_invert = nullptr;
    vistle::IntParameter *m_operation = nullptr;
    vistle::FloatParameter *m_threshold = nullptr;

    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;

    vistle::Object::ptr prepareOutputGrid(const viskores::cont::DataSet &dataset,
                                          const vistle::Object::const_ptr &inputGrid) const override;

    vistle::DataBase::ptr prepareOutputField(const viskores::cont::DataSet &dataset,
                                             const vistle::Object::const_ptr &inputGrid,
                                             const vistle::DataBase::const_ptr &inputField,
                                             const std::string &fieldName,
                                             const vistle::Object::ptr &outputGrid) const override;
};

#endif
