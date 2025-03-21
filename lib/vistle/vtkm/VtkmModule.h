#ifndef VISTLE_VTKM_VTKMMODULE_H
#define VISTLE_VTKM_VTKMMODULE_H

#include <vector>
#include <vtkm/cont/DataSet.h>
#include <vtkm/filter/Filter.h>

#include <vistle/alg/objalg.h>
#include "vistle/module/module.h"

#include "export.h"
#include "module_status.h"

//TODO: adjust class doc string
/**
 * @class VtkmModule
 * @brief Base class for VTK-m modules which allow the execution of algorithms on different device backends,
 *        including GPUs.
 *
 * By default, a VTK-m module consists of one input port and one output port. If additional ports are needed, 
 * the parent constructor can be called with the desired number (`numPorts`). Note that all data on the ports 
 * must be defined on the same grid.
 * 
 * The base behavior of a VTK-m module is as follows (see `compute()`):
 * The VTK-m module reads in the data on the input port(s), validates it and transforms it into a VTK-m dataset.
 * A VTK-m filter is then applied to the dataset, the result is transformed back to Vistle objects and finally 
 * written to the output port(s).
 * 
 * Child classes must implement the `setUpFilter` method, which defines the desired VTK-m filter. If the filter
 * only operates on the input grid, `m_requireMappedData` can be set to `false` when calling the parent constructor. 
 * Additional module parameters can be added in the child's constructor. 
 * 
 * Optionally, the child classes can override the `checkInputField` and `checkInputGrid` methods to add additional
 * checks on the input data. Moreover, `writeResultToPort` can be overridden in case data other than the output grid
 * (and field, if `m_requireMappedData` is `true`) should be written to the output port.
 * 
 * If desired, the child classes can also override the methods for transforming the input data into VTK-m datasets
 * (`transformInputGrid` and `transformInputField`) or for converting the VTK-m output back into Vistle objects
 * (`prepareOutputGrid` and `prepareOutputField`).
 */
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
        Implement to create and parameterize the VTK-m filter to be executed.
    */
    virtual std::unique_ptr<vtkm::filter::Filter> setUpFilter() const = 0;

    /*
        Reads in the grid and its data field(s) fields from the input port(s) and stores them in `grid` and `fields`.
        Also makes sure that (if there are multiple ports) all data fields are defined on the same grid.
    */
    virtual ModuleStatusPtr readInPorts(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &grid,
                                        std::vector<vistle::DataBase::const_ptr> &fields) const;

    /*
        Turns `grid` into a VTK-m grid and adds it to `dataset`.
    */
    virtual ModuleStatusPtr transformInputGrid(const vistle::Object::const_ptr &grid,
                                               vtkm::cont::DataSet &dataset) const;

    /*
        Transforms `field` to a VTK-m data set and adds it to `dataset`. The name of the field is also written to
        `fieldName`.
    */
    virtual ModuleStatusPtr transformInputField(const vistle::Port *port, const vistle::Object::const_ptr &grid,
                                                const vistle::DataBase::const_ptr &field, std::string &fieldName,
                                                vtkm::cont::DataSet &dataset) const;

    /*
        Transforms the grid in `dataset` to a Vistle data object and updates its metadata by copying the
        attributes from `inputGrid`.
    */
    virtual vistle::Object::ptr prepareOutputGrid(const vtkm::cont::DataSet &dataset,
                                                  const vistle::Object::const_ptr &inputGrid) const;
    /*
        Transform the field named `fieldName` in `dataset` to a Vistle data object and updates its metadata by
        copying the attributes from `ìnputField` and setting its grid to `outputGrid`.
    */
    virtual vistle::DataBase::ptr prepareOutputField(const vtkm::cont::DataSet &dataset,
                                                     const vistle::Object::const_ptr &inputGrid,
                                                     const vistle::DataBase::const_ptr &inputField,
                                                     const std::string &fieldName,
                                                     const vistle::Object::ptr &outputGrid) const;

    /* perform simple initial checks */
    bool prepare() override;

    /*
        Reads in the grid and its data field(s) fields from the input port(s), makes sure they are valid, transforms them
        into a VTK-m dataset. It then goes on to apply the VTK-m filter created by `setUpFilter` to the data on each port. 
        The result is transformed back to Vistle objects and finally written to the output port(s).
    */
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    /*
        Checks if `status` allows module to continue execution and (if available) prints a message to the GUI.
    */
    bool isValid(const ModuleStatusPtr &status) const;
};

#endif
