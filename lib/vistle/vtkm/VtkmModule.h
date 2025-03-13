#ifndef VISTLE_VTKM_VTKMMODULE_H
#define VISTLE_VTKM_VTKMMODULE_H

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

    /*
        Reads in the grid and its data field(s) fields from the input port(s) and stores them in `grid` and `fields`.
        Also makes sure that (if there are multiple ports) all data fields are defined on the same grid.
    */
    virtual ModuleStatusPtr readInPorts(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &grid,
                                        std::vector<vistle::DataBase::const_ptr> &fields) const;

    /*
        Checks if `grid` is a null pointer.
    */
    virtual ModuleStatusPtr checkInputGrid(const vistle::Object::const_ptr &grid) const;

    /*
        Turns `grid` into a VTK-m grid and adds it to `dataset`.
    */
    virtual ModuleStatusPtr transformInputGrid(const vistle::Object::const_ptr &grid,
                                               vtkm::cont::DataSet &dataset) const;

    /*
        Returns an error if `field` is a null pointer.
    */
    virtual ModuleStatusPtr checkInputField(const vistle::Object::const_ptr &grid,
                                            const vistle::DataBase::const_ptr &field,
                                            const std::string &portName) const;
    /*
        Transforms `field` to a VTK-m data set and adds it to `dataset`. The name of the field is also written to
        `fieldName`.
    */
    virtual ModuleStatusPtr transformInputField(const vistle::Object::const_ptr &grid,
                                                const vistle::DataBase::const_ptr &field, std::string &fieldName,
                                                vtkm::cont::DataSet &dataset) const;

    /*
        Runs a VTK-m filter on `input` and stores the result in `output`. This method must be overriden by modules
        inheriting from VtkmModule.
    */
    virtual void runFilter(const vtkm::cont::DataSet &input, const std::string &fieldName,
                           vtkm::cont::DataSet &output) const = 0;

    /*
        Transforms the grid in `dataset` to a Vistle data object and updates its metadata by copying the
        attributes from `inputGrid`.
    */
    virtual vistle::Object::ptr prepareOutputGrid(const vtkm::cont::DataSet &dataset,
                                                  const vistle::Object::const_ptr &inputGrid,
                                                  vistle::Object::ptr &outputGrid) const;
    /*
        Transform the field named `fieldName` in `dataset` to a Vistle data object and updates its metadata by
        copying the attributes from `ìnputField` and setting its grid to `outputGrid`.
    */
    virtual vistle::DataBase::ptr prepareOutputField(const vtkm::cont::DataSet &dataset,
                                                     const vistle::Object::const_ptr &inputGrid,
                                                     const vistle::DataBase::const_ptr &inputField,
                                                     const std::string &fieldName,
                                                     vistle::Object::ptr &outputGrid) const;
    /*
        Adds `outputField` to `port` if it is not a null pointer, otherwise adds `outputGrid`.
    */
    virtual void writeResultToPort(const std::shared_ptr<vistle::BlockTask> &task,
                                   const vistle::Object::const_ptr &inputGrid,
                                   const vistle::DataBase::const_ptr &inputField, vistle::Port *port,
                                   vistle::Object::ptr &outputGrid, vistle::DataBase::ptr &outputField) const;

    /*
        Reads in the grid and its data field(s) fields from the input port(s), makes sure they are valid, transforms them
        into a VTK-m dataset. It then goes on to apply the VTK-m filter called in `runFilter` to the data on each port. 
        The result is transformed back to Vistle objects and finally written to the output port(s).
    */
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    /*
        Checks if `status` allows module to continue execution and (if available) prints a message to the GUI.
    */
    bool isValid(const ModuleStatusPtr &status) const;
};

#endif
