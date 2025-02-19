#include <cstring>

#include <vistle/alg/objalg.h>

#include "convert.h"

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
    vtkm::cont::DataSet filterInput, filterOutput;
    Object::const_ptr inputGrid;
    DataBase::const_ptr inputField;

    auto status = prepareInput(task, inputGrid, inputField, filterInput);
    if (!isValid(status))
        return true;
    runFilter(filterInput, filterOutput);
    prepareOutput(task, filterOutput, inputGrid, inputField);

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
        status = vtkmAddField(result, field, m_fieldName);
        if (!isValid(status))
            return status;
    }

    return Success();
}

ModuleStatusPtr VtkmModule::prepareInput(const std::shared_ptr<BlockTask> &task, Object::const_ptr &inputGrid,
                                         DataBase::const_ptr &inputField, vtkm::cont::DataSet &filterInputData) const
{
    auto status = getGridFromPort(m_dataIn, task, inputGrid, m_requireMappedData, inputField, m_fieldName);
    if (!isValid(status))
        return status;

    status = inputToVtkm(inputGrid, inputField, filterInputData);
    if (!isValid(status))
        return status;

    return Success();
}

void copyGridMetaData(const Object::const_ptr &copyFrom, Object::ptr &copyTo)
{
    copyTo->copyAttributes(copyFrom);
    copyTo->setTransform(copyFrom->getTransform());
    if (copyTo->getTimestep() < 0) {
        copyTo->setTimestep(copyFrom->getTimestep());
        copyTo->setNumTimesteps(copyFrom->getNumTimesteps());
    }
    if (copyTo->getBlock() < 0) {
        copyTo->setBlock(copyFrom->getBlock());
        copyTo->setNumBlocks(copyFrom->getNumBlocks());
    }
}
bool VtkmModule::prepareOutput(const std::shared_ptr<BlockTask> &task, vtkm::cont::DataSet &filterOutputData,
                               Object::const_ptr &inputGrid, DataBase::const_ptr &inputField) const
{
    Object::ptr geoOut = vtkmGetGeometry(filterOutputData);
    if (!geoOut) {
        sendError("An error occurred while transforming the filter output grid to a Vistle object.");
        return true;
    }

    updateMeta(geoOut);
    copyGridMetaData(inputGrid, geoOut);

    if (m_requireMappedData) {
        if (auto mapped = vtkmGetField(filterOutputData, m_fieldName)) {
            std::cerr << "mapped data: " << *mapped << std::endl;

            mapped->copyAttributes(inputField);
            mapped->setGrid(geoOut);
            updateMeta(mapped);

            // add output grid + mapped data to output port
            task->addObject(m_dataOut, mapped);
            return true;
        } else {
            sendError("An error occurred while transforming the filter output field to a Vistle object.");
        }
    }

    // if there is no mapped data, add output grid to output port
    task->addObject(m_dataOut, geoOut);
    return true;
}
