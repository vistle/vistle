#ifndef VISTLE_VTKM_VTKM_MODULE_H
#define VISTLE_VTKM_VTKM_MODULE_H

#include <string>
#include <vector>
#include <viskores/cont/DataSet.h>
#include <viskores/filter/Filter.h>

#include <vistle/alg/objalg.h>
#include <vistle/module/module.h>
#include <vistle/util/enum.h>

#include "export.h"
#include "module_status.h"

namespace vistle {

/**
 * @class VtkmModule
 * @brief Base class for Viskores modules which allow the execution of algorithms on different device backends,
 *        including GPUs.
 *
 * By default, a Viskores module consists of one input port and one output port. If additional ports are needed,
 * the parent constructor can be called with the desired number (`numPorts`). Note that all data on the ports
 * must be defined on the same grid.
 *
 * The base behavior of a Viskores module is as follows (see `compute()`):
 * The Viskores module reads in the data on the input port(s) and transforms it into a Viskores dataset. A Viskores filter,
 * which uses the data on the first port as active field, is then applied to the dataset. The result is transformed
 * back to Vistle objects and finally written to the output port(s).
 *
 * Child classes must implement the `setUpFilter` method, which defines the desired Viskores filter. If the filter
 * only operates on the input grid, `m_requireMappedData` can be set to `false` when calling the parent constructor.
 * Additional module parameters can be added in the child's constructor.
 *
 * Optionally, the child classes can override the `prepareInputGrid` and `prepareInputField` methods to add checks
 * on the input data, before transforming it into the Viskores format. `prepareOutputGrid` and `prepareOutputField`
 * can be overridden, if desired, to change the output grid or field(s) that will be added to the output
 * port(s).
 */
class V_VTKM_EXPORT VtkmModule: public Module {
public:
    struct InputData {
        vistle::Object::const_ptr vistleGrid;
        std::vector<vistle::DataBase::const_ptr> fields;

        viskores::cont::DataSet viskoresDataset;
    };

    struct OutputData {
        vistle::Object::const_ptr vistleGrid;
        std::vector<vistle::DataBase::ptr> fields;

        viskores::cont::DataSet viskoresDataset;
    };

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(MappedDataHandling, (Use)(Require)(Discard)(Generate))

    VtkmModule(const std::string &name, int moduleID, mpi::communicator comm, int numPorts = 1,
               MappedDataHandling mode = MappedDataHandling::Require);

protected:
    const int m_numPorts;
    const MappedDataHandling m_mappedDataHandling;

    std::vector<vistle::Port *> m_inputPorts, m_outputPorts;

    /*
        Returns the name of the field at port `index`, `output` determines whether the name is
        for an input or an output field.
    */
    virtual std::string getFieldName(int index, bool output = false) const;

    /*
        Implement to create and parameterize the Viskores filter to be executed.
    */
    virtual std::unique_ptr<viskores::filter::Filter> setUpFilter(const InputData &input) const = 0;

    /*
        Transforms the input Vistle grid into a Viskores dataset.
    */
    virtual ModuleStatusPtr prepareInputGrid(InputData &input) const;

    /*
        Transforms the input field at port `port` with the index `index` into a Viskores field and adds
        it to the dataset in `input`.
    */
    virtual ModuleStatusPtr prepareInputField(const vistle::Port *port, InputData &input, int index) const;

    /*
        Transforms the output grid into a Vistle data object, adds it to `output` and updates its metadata
        by copying the attributes from the grid in `input`. The output grid will be added to the output
        port by `compute` if no output field is defined.
    */
    virtual vistle::Object::const_ptr prepareOutputGrid(const InputData &input, OutputData &output) const;

    /*
        Transforms the output field named `fieldName` and with the index `index` into a Vistle data object.
        It also updates the field's metadata by copying the attributes from the corresponding input field
        and, if defined, sets its grid to `the grid in `output`. The output field will be added to the output
        port in `compute` if it is not null.
    */
    virtual vistle::DataBase::ptr prepareOutputField(const InputData &input, OutputData &output, int index,
                                                     const std::string &fieldName) const;

    /*
        Initial sanity check for the connected input and output ports.
     */
    bool prepare() override;

    /*
        Reads in the grid and its data field(s) from the input port(s) and stores them in `grid` and `fields`.
        Also makes sure all data fields are defined on the same grid.
    */
    ModuleStatusPtr readInPorts(const std::shared_ptr<vistle::BlockTask> &task, vistle::Object::const_ptr &grid,
                                std::vector<vistle::DataBase::const_ptr> &fields) const;

    /*
        Reads in the grid and its data field(s) from the input port(s), makes sure they are valid and transforms
        them into a Viskores dataset. It then goes on to apply the Viskores filter created by `setUpFilter` to
        the data on each port. The result is transformed back to Vistle objects and finally written to the output
        port(s).
    */
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    /*
        Checks if `status` allows module to continue execution and (if available) prints a message to the GUI.
    */
    bool checkAndNotify(const ModuleStatusPtr &status) const;

    /*
        Attempts to execute the given Viskores filter on the input dataset and handles any exceptions that may occur.
        If an exception is thrown, a message will be printed to the GUI and the method will return false.
    */
    bool tryToExecuteFilter(viskores::filter::Filter &filter, const viskores::cont::DataSet &inputDataset,
                            viskores::cont::DataSet &outputDataset) const;

private:
    vistle::IntParameter *m_printObjectInfo = nullptr;
};
} // namespace vistle
#endif
