#ifndef VISTLE_VECTORFIELDVTKM_VECTORFIELDVTKM_H
#define VISTLE_VECTORFIELDVTKM_VECTORFIELDVTKM_H

#include <vistle/vtkm/vtkm_module.h>
#include <vistle/util/enum.h>



class VectorFieldVtkm : public vistle::VtkmModule {
public:
    VectorFieldVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;

    vistle::Object::const_ptr prepareOutputGrid(
        const viskores::cont::DataSet &dataset,
        const vistle::Object::const_ptr &inputGrid) const override;

    vistle::DataBase::ptr prepareOutputField(
        const viskores::cont::DataSet &dataset,
        const vistle::Object::const_ptr &inputGrid,
        const vistle::DataBase::const_ptr &inputField,
        const std::string &fieldName,
        const vistle::Object::const_ptr &outputGrid) const override;

    vistle::FloatParameter  *m_scale      = nullptr;
    vistle::VectorParameter *m_range      = nullptr;
    vistle::IntParameter    *m_attachment = nullptr;
    vistle::IntParameter    *m_allCoords  = nullptr;
    mutable vistle::Index m_numLines = 0;
};

#endif // VISTLE_VECTORFIELDVTKM_VECTORFIELDVTKM_H