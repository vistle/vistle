#ifndef VISTLE_VTKM_VTKMMODULE_H
#define VISTLE_VTKM_VTKMMODULE_H

#include <vector>
#include <vtkm/cont/DataSet.h>
#include <vtkm/filter/Filter.h>

#include <vistle/alg/objalg.h>
#include "vistle/module/module.h"

#include "export.h"
#include "module_status.h"

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
 * The VTK-m module reads in the data on the input port(s) and transforms it into a VTK-m dataset. A VTK-m filter,
 * which uses the data on the first port as active field, is then applied to the dataset. The result is transformed
 * back to Vistle objects and finally written to the output port(s).
 * 
 * Child classes must implement the `setUpFilter` method, which defines the desired VTK-m filter. If the filter
 * only operates on the input grid, `m_requireMappedData` can be set to `false` when calling the parent constructor. 
 * Additional module parameters can be added in the child's constructor. 
 * 
 * Optionally, the child classes can override the `prepareInputGrid` and `prepareInputField` methods to add checks 
 * on the input data, before transforming it into the VTK-m format. `prepareOutputGrid` and `prepareOutputField`
 * can be overridden, if desired, to change the output grid or field(s) that will be added to the output
 * port(s).
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
        Also makes sure all data fields are defined on the same grid.
    */
    virtual ModuleStatusPtr readInPorts(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &grid,
                                        std::vector<vistle::DataBase::const_ptr> &fields) const;

    /*
        Turns `grid` into a VTK-m grid and adds it to `dataset`.
    */
    virtual ModuleStatusPtr prepareInputGrid(const vistle::Object::const_ptr &grid, vtkm::cont::DataSet &dataset) const;

    /*
        Transforms `field` to a VTK-m field with the name `fieldName` and adds it to `dataset`.
    */
    virtual ModuleStatusPtr prepareInputField(const vistle::Port *port, const vistle::Object::const_ptr &grid,
                                              const vistle::DataBase::const_ptr &field, std::string &fieldName,
                                              vtkm::cont::DataSet &dataset) const;

    /*
        Transforms the grid in `dataset` to a Vistle data object and updates its metadata by copying the
        attributes from `inputGrid`. The returned grid will be added to the output port in `compute` if
        no output field is defined.
    */
    virtual vistle::Object::ptr prepareOutputGrid(const vtkm::cont::DataSet &dataset,
                                                  const vistle::Object::const_ptr &inputGrid) const;
    /*
        Transform the field named `fieldName` in `dataset` to a Vistle data object and updates its metadata by
        copying the attributes from `ìnputField` and, if defined, setting its grid to `outputGrid`. The returned
        field will be added to the output port in `compute` if it is not null.
    */
    virtual vistle::DataBase::ptr prepareOutputField(const vtkm::cont::DataSet &dataset,
                                                     const vistle::Object::const_ptr &inputGrid,
                                                     const vistle::DataBase::const_ptr &inputField,
                                                     const std::string &fieldName,
                                                     const vistle::Object::ptr &outputGrid) const;

    /* 
        Initial sanity check for the connected input and output ports.
     */
    bool prepare() override;

    /*
        Reads in the grid and its data field(s) fields from the input port(s), makes sure they are valid and transforms
        them into a VTK-m dataset. It then goes on to apply the VTK-m filter created by `setUpFilter` to the data on each
        port. The result is transformed back to Vistle objects and finally written to the output port(s).
    */
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    /*
        Checks if `status` allows module to continue execution and (if available) prints a message to the GUI.
    */
    bool isValid(const ModuleStatusPtr &status) const;
};

#endif
