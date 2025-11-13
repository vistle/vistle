#ifndef VISTLE_TOTRIANGLESVTKM_TOTRIANGLESVTKM_H
#define VISTLE_TOTRIANGLESVTKM_TOTRIANGLESVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class ToTrianglesVtkm: public vistle::VtkmModule {
public:
    ToTrianglesVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    mutable bool applyFilter;

    ModuleStatusPtr prepareInputGrid(const vistle::Object::const_ptr &grid,
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

#endif // VISTLE_TOTRIANGLESVTKM_TOTRIANGLESVTKM_H
