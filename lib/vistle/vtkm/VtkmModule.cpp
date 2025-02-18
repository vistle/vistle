#include <cstring>

#include <vistle/alg/objalg.h>

#include "convert.h"
#include "module_status.h"

#include "VtkmModule.h"

using namespace vistle;


//TODO: make all VTKm modules inherit from this class
//TODO: should isValid() be moved to the Module class as it is not specific to VTKm?

VtkmModule::VtkmModule(const std::string &name, int moduleID, mpi::communicator comm, bool requireMappedData)
: Module(name, moduleID, comm), m_requireMappedData(requireMappedData)
{
    m_dataIn = createInputPort("data_in", m_requireMappedData ? "input grid with mapped data" : "input grid");
    m_dataOut = createOutputPort("data_out", m_requireMappedData ? "output grid with mapped data" : "output grid");
}

VtkmModule::~VtkmModule()
{}

bool VtkmModule::isValid(const ModuleStatusPtr &status) const
{
    if (strcmp(status->message(), ""))
        sendText(status->messageType(), status->message());

    return status->continueExecution();
}

bool VtkmModule::compute(const std::shared_ptr<BlockTask> &task) const
{
    vtkm::cont::DataSet filterInputData, filterOutputData;
    Object::const_ptr inputGrid;
    DataBase::const_ptr inputField;

    auto status = prepareInput(task, inputGrid, inputField, filterInputData);
    if (!isValid(status))
        return true;
    runFilter(filterInputData, filterOutputData);
    prepareOutput(task, filterOutputData, inputGrid, inputField);

    return true;
}

ModuleStatusPtr getGridFromPort(const Port *port, const std::shared_ptr<BlockTask> &task, Object::const_ptr &resultGrid,
                                bool readField, DataBase::const_ptr &resultField, std::string &resultFieldName)
{
    auto container = task->accept<Object>(port);
    auto split = splitContainerObject(container);

    resultGrid = split.geometry;
    if (!resultGrid)
        return Error("Input grid is missing!");

    if (readField) {
        resultField = split.mapped;
        if (!resultField)
            return Error("No mapped data on input grid!");

        // if the mapped data on the input port already has a name, keep it
        if (auto name = resultField->getAttribute("_species"); !name.empty())
            resultFieldName = name;
    }

    return Success();
}

ModuleStatusPtr VtkmModule::inputToVtkm(Object::const_ptr &grid, DataBase::const_ptr &field,
                                        vtkm::cont::DataSet &result) const
{
    auto status = vtkmSetGrid(result, grid);
    if (!isValid(status))
        return status;

    if (m_requireMappedData) {
        status = vtkmAddField(result, field, m_mappedDataName);
        if (!isValid(status))
            return status;
    }

    return Success();
}

ModuleStatusPtr VtkmModule::prepareInput(const std::shared_ptr<BlockTask> &task, Object::const_ptr &inputGrid,
                                         DataBase::const_ptr &inputField, vtkm::cont::DataSet &filterInputData) const
{
    auto status = getGridFromPort(m_dataIn, task, inputGrid, m_requireMappedData, inputField, m_mappedDataName);
    if (!isValid(status))
        return status;

    status = inputToVtkm(inputGrid, inputField, filterInputData);
    if (!isValid(status))
        return status;

    return Success();
}

bool VtkmModule::prepareOutput(const std::shared_ptr<BlockTask> &task, vtkm::cont::DataSet &filterOutputData,
                               Object::const_ptr &inputGrid, DataBase::const_ptr &inputField) const
{
    // transform result back to Vistle
    Object::ptr geoOut = vtkmGetGeometry(filterOutputData);

    // add result to output
    if (geoOut) {
        updateMeta(geoOut);
        geoOut->copyAttributes(inputGrid);
        geoOut->setTransform(inputGrid->getTransform());
        if (geoOut->getTimestep() < 0) {
            geoOut->setTimestep(inputGrid->getTimestep());
            geoOut->setNumTimesteps(inputGrid->getNumTimesteps());
        }
        if (geoOut->getBlock() < 0) {
            geoOut->setBlock(inputGrid->getBlock());
            geoOut->setNumBlocks(inputGrid->getNumBlocks());
        }
    }

    if (m_requireMappedData) {
        if (auto mapped = vtkmGetField(filterOutputData, m_mappedDataName)) {
            std::cerr << "mapped data: " << *mapped << std::endl;
            mapped->copyAttributes(inputField);
            mapped->setGrid(geoOut);
            updateMeta(mapped);
            task->addObject(m_dataOut, mapped);
            return true;
        } else {
            sendError("could not handle mapped data");
            task->addObject(m_dataOut, geoOut);
        }
    } else {
        task->addObject(m_dataOut, geoOut);
    }
    return true;
}
