#ifndef VISTLE_VTKM_VTKMMODULE_H
#define VISTLE_VTKM_VTKMMODULE_H

#include <vtkm/cont/DataSet.h>

#include <vistle/alg/objalg.h>
#include "vistle/module/module.h"

#include "export.h"
#include "module_status.h"

/**
 * @class VtkmModule
 * @brief Base class for VTK-m modules which allows the execution of algorithms on the GPU.
 *
 * A VTK-m module consists of one input port `m_dataIn` containing the input grid for the VTK-m filter and 
 * one output port `m_dataOut` with the filter's output. Child classes must override the `runFilter` method
 * where the desired VTK-m filter is called. 
 * 
 * If the VTK-m filter does not need any data field on the input grid, set `m_requireMappedData` to false
 * in the constructor. Moreover, module parameters can be added in the child's constructor. 
 * 
 * The transformation from Vistle objects to VTK-m datasets and vice versa is handled automatically in the 
 * `prepareInput` and `prepareOutput` methods. These two methods can be  overriden in case the child class 
 * needs to create and handle additional input or output ports.
 * 
 */
class V_VTKM_EXPORT VtkmModule: public vistle::Module {
public:
    VtkmModule(const std::string &name, int moduleID, mpi::communicator comm, bool requireMappedData = true);
    ~VtkmModule();

protected:
    vistle::Port *m_dataIn, *m_dataOut;

    bool m_requireMappedData;
    mutable std::string m_fieldName = "mappedData";

    /*
        Reads the grid (and mapped data, if `m_requireMappedData` is true) from the input port `m_dataIn`, 
        stores them in `grid` (and `field`), and transforms them into the VTK-m dataset `dataset`.
    */
    virtual ModuleStatusPtr prepareInput(const std::shared_ptr<vistle::BlockTask> &task,
                                         vistle::Object::const_ptr &grid, vistle::DataBase::const_ptr &field,
                                         vtkm::cont::DataSet &dataset) const;

    /*
        Runs a VTK-m filter on `filterInput` and stores the result in `filterOutput`. 
        This method must be overriden by modules inheriting from VtkmModule.
    */
    virtual void runFilter(vtkm::cont::DataSet &filterInput, vtkm::cont::DataSet &filterOutput) const = 0;

    /*
        Transforms the VTK-m dataset `dataset` into a Vistle grid (and field, if `m_requireMappedData` 
        is true, updates the metadata (copying the metadata from the input port) and adds it to the
        output port `m_dataOut`.
    */
    virtual bool prepareOutput(const std::shared_ptr<vistle::BlockTask> &task, vtkm::cont::DataSet &dataset,
                               vistle::Object::ptr &outputGrid, vistle::Object::const_ptr &inputGrid,
                               vistle::DataBase::const_ptr &inputField) const;

    /*
        Reads the grid stored in `split`, i.e., the data object provided by an input port, into `grid`
        and transforms it into the VTK-m dataset `dataset`.
    */
    ModuleStatusPtr prepareInputGrid(const vistle::DataComponents &split, vistle::Object::const_ptr &grid,
                                     vtkm::cont::DataSet &dataset, const std::string &portName = "") const;

    /* 
        Reads the field stored in `split`, i.e., the data object provided by an input port, into `field`
        and adds it to the VTK-m dataset `dataset`. If the mapped data on the input port has a name, it 
        will overwrite `fieldName` with it.
    */
    ModuleStatusPtr prepareInputField(const vistle::DataComponents &split, vistle::DataBase::const_ptr &field,
                                      std::string &fieldName, vtkm::cont::DataSet &dataset,
                                      const std::string &portName = "") const;

    /*
        Transform the grid in `dataset` into a Vistle object, updates its metadata and returns it.
    */
    vistle::Object::ptr prepareOutputGrid(vtkm::cont::DataSet &dataset) const;

    /*
        Transform the field in `dataset` into a Vistle object, updates its metadata and returns it.
    */
    vistle::DataBase::ptr prepareOutputField(vtkm::cont::DataSet &dataset, const std::string &fieldName) const;

    /*
        Checks if module can continue its execution and (optionally) prints a message to the GUI.
    */
    bool isValid(const ModuleStatusPtr &status) const;

    /*
        Calls the `prepareInput`, `runFilter`, and `prepareOutput` methods in this order every time
        the execution of the module is triggered, e.g., through the GUI.
    */
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif // VISTLE_VTKM_VTKMMODULE_H
