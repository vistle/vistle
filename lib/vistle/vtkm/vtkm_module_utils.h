#ifndef VISTLE_VTKM_VTKM_MODULE_UTILS_H
#define VISTLE_VTKM_VTKM_MODULE_UTILS_H

#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/filter/Filter.h>

#include <vistle/module/module.h>

#include "module_status.h"

namespace vistle {
namespace vtkm {
/*
    @brief Checks if the module status is valid and sends a message to the GUI if necessary.

    This function checks if the provided ModuleStatus object indicates that the module can
    continue its execution. If the status contains a message, it sends the message to the
    GUI using the Module's send-methods.

    @return True if the module can continue its execution, false otherwise.
*/
bool V_VTKM_EXPORT checkAndNotify(const vistle::Module &module, const ModuleStatusPtr &status);

/*
    @brief Attempts to execute a Viskores filter and handles any exceptions that may occur.

    This function attempts to execute the provided Viskores filter on the input dataset. If
    an exception occurs during execution, it captures the exception details.

    @return `Success` if the filter was executed successfully, `Error` with the captured details
    otherwise.
*/
ModuleStatusPtr V_VTKM_EXPORT tryToExecuteFilter(viskores::filter::Filter &filter,
                                                 const viskores::cont::DataSet &inputDataset,
                                                 viskores::cont::DataSet &outputDataset);

/*
    @brief Attempts to execute a Viskores filter and handles any exceptions that may occur.

    This function attempts to execute the provided Viskores filter on the input dataset. If
    an exception occurs during execution, it captures the exception details and sends an
    appropriate message to the GUI using the Module's send-methods.

    @return True if the filter was executed successfully, false otherwise.
*/
bool V_VTKM_EXPORT tryToExecuteFilter(const vistle::Module &module, viskores::filter::Filter &filter,
                                      const viskores::cont::DataSet &inputDataset,
                                      viskores::cont::DataSet &outputDataset);

/*
    @brief Attempts to execute a Viskores filter and handles any exceptions that may occur.

    This function attempts to execute the provided Viskores filter on the partitioned input
    dataset. If an exception occurs during execution, it captures the exception details.

    @return `Success` if the filter was executed successfully, `Error` with the captured details
    otherwise.
*/
ModuleStatusPtr V_VTKM_EXPORT tryToExecuteFilter(viskores::filter::Filter &filter,
                                                 const viskores::cont::PartitionedDataSet &inputDataset,
                                                 viskores::cont::PartitionedDataSet &outputDataset);

/*
    @brief Attempts to execute a Viskores filter and handles any exceptions that may occur.

    This function attempts to execute the provided Viskores filter on the partitioned input
    dataset. If an exception occurs during execution, it captures the exception details and
    sends an appropriate message to the GUI using the Module's send-methods.

    @return True if the filter was executed successfully, false otherwise.
*/
bool V_VTKM_EXPORT tryToExecuteFilter(const vistle::Module &module, viskores::filter::Filter &filter,
                                      const viskores::cont::PartitionedDataSet &inputDataset,
                                      viskores::cont::PartitionedDataSet &outputDataset);
} // namespace vtkm
} // namespace vistle

#endif // VISTLE_VTKM_VTKM_MODULE_UTILS_H
