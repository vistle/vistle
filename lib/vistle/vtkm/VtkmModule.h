#ifndef VISTLE_VTKM_VTKMMODULE_H
#define VISTLE_VTKM_VTKMMODULE_H

#include <vtkm/cont/DataSet.h>

#include <vistle/module/module.h>

class V_VTKM_EXPORT VtkmModule: public vistle::Module {
public:
    VtkmModule(const std::string &name, int moduleID, mpi::communicator comm);
    ~VtkmModule();

protected:
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    std::unique_ptr<ConvertStatus> inputToVistle(const std::shared_ptr<vistle::BlockTask> &task,
                                                 vtkm::cont::DataSet &inputData, vistle::Object::const_ptr &grid,
                                                 vistle::DataBase::const_ptr &field) const;
    bool outputToVtkm(const std::shared_ptr<vistle::BlockTask> &task, vtkm::cont::DataSet &outputData,
                      vistle::Object::const_ptr &grid, vistle::DataBase::const_ptr &field) const;
    virtual void runFilter(vtkm::cont::DataSet &inputData, vtkm::cont::DataSet &outputData) const = 0;

    bool checkStatus(const std::unique_ptr<ConvertStatus> &status) const;

    vistle::Port *m_mapDataIn, *m_dataOut;

    mutable std::string m_mapSpecies;
};

#endif // VISTLE_VTKM_VTKMMODULE_H