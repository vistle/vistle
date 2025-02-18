#ifndef VISTLE_VTKM_VTKMMODULE_H
#define VISTLE_VTKM_VTKMMODULE_H

#include <vtkm/cont/DataSet.h>

#include <vistle/module/module.h>

/**
 * @class VtkmModule
 * @brief Base class for VTK-m modules which allows the execution of algorithms on the GPU.
 *
 * In its base form, a VTK-m module consists of one input port containing the input grid for the VTK-m
 * filter and one output port with the filter's output. If the VTK-m filter expects data fields to be 
 * present on the input grid, `m_requireMappedData` should be set to true in the constructor. Child 
 * classes must override the `runFilter` method.
 * 
 * The transformation from Vistle objects to VTK-m datasets and vice versa is handled automatically in the
 * `prepareInput` and `prepareOutput` methods. Child classes can add more ports in their constructors if
 * necessary. In this case, the base `prepareInput` and `prepareOutput` methods should be extended in the
 * child class to handle the additional ports.
 */
class V_VTKM_EXPORT VtkmModule: public vistle::Module {
public:
    VtkmModule(const std::string &name, int moduleID, mpi::communicator comm, bool requireMappedData = true);
    ~VtkmModule();

protected:
    vistle::Port *m_dataIn, *m_dataOut;

    bool m_requireMappedData;
    mutable std::string m_mappedDataName = "mappedData";

    /*
        Calls the `prepareInput`, `runFilter`, and `prepareOutput` methods in this order every time
        the execution of the module is triggered, e.g., through the GUI.
    */
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    ModuleStatusPtr inputToVtkm(vistle::Object::const_ptr &grid, vistle::DataBase::const_ptr &field,
                                vtkm::cont::DataSet &result) const;

    // Checks if the data on the input port is valid and transforms it into a VTK-m dataset.
    ModuleStatusPtr prepareInput(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &inputGrid,
                                 vistle::DataBase::const_ptr &inputField, vtkm::cont::DataSet &filterInputData) const;
    /*
        Runs a VTK-m filter on `filterInputData` and stores the result in `filterOutputData`. 
        This method must be overriden by modules inheriting from VtkmModule.
    */
    virtual void runFilter(vtkm::cont::DataSet &filterInputData, vtkm::cont::DataSet &filterOutputData) const = 0;

    // Transforms VTK-m filter output into Vistle objects and adds them to the output port.
    bool prepareOutput(const std::shared_ptr<vistle::BlockTask> &task, vtkm::cont::DataSet &filterOutputData,
                       vistle::Object::const_ptr &inputGrid, vistle::DataBase::const_ptr &inputField) const;


    // Checks if module can continue its execution and (optionally) prints a debug message to the GUI.
    bool isValid(const ModuleStatusPtr &status) const;
};

#endif // VISTLE_VTKM_VTKMMODULE_H
