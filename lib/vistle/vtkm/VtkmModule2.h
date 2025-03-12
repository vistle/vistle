#ifndef VTKM_MODULE2_H
#define VTKM_MODULE2_H

#include <vector>
#include <vtkm/cont/DataSet.h>

#include <vistle/alg/objalg.h>
#include "vistle/module/module.h"

#include "export.h"
#include "module_status.h"

//TODO: requireMappedData!

class V_VTKM_EXPORT VtkmModule: public vistle::Module {
public:
    VtkmModule2(const std::string &name, int moduleID, mpi::communicator comm, int numPorts = 1,
                bool requireMappedData = true);
    ~VtkmModule2();

private:
    const int m_numPorts;

    std::vector<vistle::Port *> m_inputPorts, m_outputPorts;

    ModuleStatusPtr CheckGrid(const Object::const_ptr &grid);
    ModuleStatusPtr ReadInPorts(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &grid,
                                std::vector<vistle::DataBase::const_ptr> &fields) const;

    void runFilter(vtkm::cont::DataSet &input, std::string &fieldName, vtkm::cont::DataSet &output) const;

    bool isValid(const ModuleStatusPtr &status) const;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif // VTKM_MODULE2_H