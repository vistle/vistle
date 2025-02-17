#ifndef VISTLE_VTKM_VTKMMODULE_H
#define VISTLE_VTKM_VTKMMODULE_H

#include <vtkm/cont/DataSet.h>

#include <vistle/module/module.h>

class V_VTKM_EXPORT VtkmModule: public vistle::Module {
public:
    VtkmModule(const std::string &name, int moduleID, mpi::communicator comm, bool requireMappedData = true);
    ~VtkmModule();

protected:
    vistle::Port *m_dataIn, *m_dataOut;

    bool m_requireMappedData;
    mutable std::string m_mappedDataName;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    ModuleStatusPtr inputToVtkm(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &inputGrid,
                                vistle::DataBase::const_ptr &inputField, vtkm::cont::DataSet &filterInputData) const;
    bool outputToVistle(const std::shared_ptr<vistle::BlockTask> &task, vtkm::cont::DataSet &filterOutputData,
                        vistle::Object::const_ptr &inputGrid, vistle::DataBase::const_ptr &inputField) const;
    virtual void runFilter(vtkm::cont::DataSet &filterInputData, vtkm::cont::DataSet &filterOutputData) const = 0;

    bool canContinueExecution(const ModuleStatusPtr &status) const;
};

#endif // VISTLE_VTKM_VTKMMODULE_H
