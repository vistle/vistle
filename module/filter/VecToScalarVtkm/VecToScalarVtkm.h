#ifndef VISTLE_VECTOSCALARVTKM_VECTOSCALARVTKM_H
#define VISTLE_VECTOSCALARVTKM_VECTOSCALARVTKM_H

#include <vistle/vtkm/vtkm_module.h>


class VecToScalarVtkm: public vistle::VtkmModule {
public:
    VecToScalarVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~VecToScalarVtkm();

private:
    vistle::IntParameter *m_caseParam;
    mutable viskores::filter::Filter *filter_ = nullptr;

    ModuleStatusPtr prepareInputField(const vistle::Port *port, const vistle::Object::const_ptr &grid,
                                      const vistle::DataBase::const_ptr &field, std::string &fieldName,
                                      viskores::cont::DataSet &dataset) const override;


    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;

    vistle::Object::const_ptr prepareOutputGrid(const viskores::cont::DataSet &dataset,
                                                const vistle::Object::const_ptr &inputGrid) const override;

    vistle::DataBase::ptr prepareOutputField(const viskores::cont::DataSet &dataset,
                                             const vistle::Object::const_ptr &inputGrid,
                                             const vistle::DataBase::const_ptr &inputField,
                                             const std::string &fieldName,
                                             const vistle::Object::const_ptr &outputGrid) const override;
};

#endif
