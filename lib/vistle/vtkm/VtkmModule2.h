#ifndef VTKM_MODULE2_H
#define VTKM_MODULE2_H

#include <vector>
#include <vtkm/cont/DataSet.h>

#include <vistle/alg/objalg.h>
#include "vistle/module/module.h"

#include "export.h"
#include "module_status.h"

//TODO: requireMappedData!

class V_VTKM_EXPORT VtkmModule2: public vistle::Module {
public:
    VtkmModule2(const std::string &name, int moduleID, mpi::communicator comm, int numPorts = 1,
                bool requireMappedData = true);
    ~VtkmModule2();

private:
    const int m_numPorts;
    const bool m_requireMappedData;

    std::vector<vistle::Port *> m_inputPorts, m_outputPorts;

    ModuleStatusPtr CheckGrid(const vistle::Object::const_ptr &grid) const;
    ModuleStatusPtr TransformGrid(const vistle::Object::const_ptr &grid, vtkm::cont::DataSet &dataset) const;
    ModuleStatusPtr prepareInputGrid(const vistle::Object::const_ptr &grid, vtkm::cont::DataSet &dataset) const;

    ModuleStatusPtr CheckField(const vistle::Object::const_ptr &grid, const vistle::DataBase::const_ptr &field,
                               const std::string &portName) const;
    ModuleStatusPtr TransformField(const vistle::Object::const_ptr &grid, const vistle::DataBase::const_ptr &field,
                                   std::string &fieldName, vtkm::cont::DataSet &dataset) const;
    ModuleStatusPtr prepareInputField(const vistle::Object::const_ptr &grid, const vistle::DataBase::const_ptr &field,
                                      std::string &fieldName, vtkm::cont::DataSet &dataset,
                                      const std::string &portName) const;
    ModuleStatusPtr ReadInPorts(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &grid,
                                std::vector<vistle::DataBase::const_ptr> &fields) const;

    virtual void runFilter(vtkm::cont::DataSet &input, std::string &fieldName, vtkm::cont::DataSet &output) const = 0;

    bool prepareOutput(const std::shared_ptr<vistle::BlockTask> &task, vistle::Port *port, vtkm::cont::DataSet &dataset,
                       vistle::Object::ptr &outputGrid, vistle::Object::const_ptr &inputGrid,
                       vistle::DataBase::const_ptr &inputField, std::string &fieldName,
                       vistle::DataBase::ptr &outputField) const;

    vistle::Object::ptr prepareOutputGrid(vtkm::cont::DataSet &dataset, const vistle::Object::const_ptr &inputGrid,
                                          vistle::Object::ptr &outputGrid) const;
    vistle::DataBase::ptr prepareOutputField(vtkm::cont::DataSet &dataset,
                                             const vistle::DataBase::const_ptr &inputField, std::string &fieldName,
                                             const vistle::Object::const_ptr &inputGrid,
                                             vistle::Object::ptr &outputGrid) const;


    bool isValid(const ModuleStatusPtr &status) const;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif // VTKM_MODULE2_H
