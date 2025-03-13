#ifndef VTKM_MODULE_H
#define VTKM_MODULE_H

#include <vector>
#include <vtkm/cont/DataSet.h>

#include <vistle/alg/objalg.h>
#include "vistle/module/module.h"

#include "export.h"
#include "module_status.h"

class V_VTKM_EXPORT VtkmModule: public vistle::Module {
public:
    VtkmModule(const std::string &name, int moduleID, mpi::communicator comm, int numPorts = 1,
               bool requireMappedData = true);
    ~VtkmModule();

protected:
    const int m_numPorts;
    const bool m_requireMappedData;

    std::vector<vistle::Port *> m_inputPorts, m_outputPorts;

    virtual ModuleStatusPtr readInPorts(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &grid,
                                        std::vector<vistle::DataBase::const_ptr> &fields) const;

    virtual ModuleStatusPtr checkInputGrid(const vistle::Object::const_ptr &grid) const;
    virtual ModuleStatusPtr transformInputGrid(const vistle::Object::const_ptr &grid,
                                               vtkm::cont::DataSet &dataset) const;

    virtual ModuleStatusPtr checkInputField(const vistle::Object::const_ptr &grid,
                                            const vistle::DataBase::const_ptr &field,
                                            const std::string &portName) const;
    virtual ModuleStatusPtr transformInputField(const vistle::Object::const_ptr &grid,
                                                const vistle::DataBase::const_ptr &field, std::string &fieldName,
                                                vtkm::cont::DataSet &dataset) const;

    virtual void runFilter(const vtkm::cont::DataSet &input, const std::string &fieldName,
                           vtkm::cont::DataSet &output) const = 0;

    virtual vistle::Object::ptr prepareOutputGrid(const vtkm::cont::DataSet &dataset,
                                                  const vistle::Object::const_ptr &inputGrid,
                                                  vistle::Object::ptr &outputGrid) const;
    virtual vistle::DataBase::ptr prepareOutputField(const vtkm::cont::DataSet &dataset,
                                                     const vistle::Object::const_ptr &inputGrid,
                                                     const vistle::DataBase::const_ptr &inputField,
                                                     const std::string &fieldName,
                                                     vistle::Object::ptr &outputGrid) const;
    virtual void writeResultToPort(const std::shared_ptr<vistle::BlockTask> &task,
                                   const vistle::Object::const_ptr &inputGrid,
                                   const vistle::DataBase::const_ptr &inputField, vistle::Port *port,
                                   vistle::Object::ptr &outputGrid, vistle::DataBase::ptr &outputField) const;

    bool isValid(const ModuleStatusPtr &status) const;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif // VTKM_MODULE_H
