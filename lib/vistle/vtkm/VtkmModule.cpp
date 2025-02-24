#include <cstring>
#include <sstream>

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
    Object::const_ptr inputGrid;
    Object::ptr outputGrid;
    DataBase::const_ptr inputField;

    vtkm::cont::DataSet filterInput, filterOutput;

    auto status = prepareInput(task, inputGrid, inputField, filterInput);
    if (!isValid(status))
        return true;
    runFilter(filterInput, filterOutput);
    prepareOutput(task, filterOutput, outputGrid, inputGrid, inputField);

    return true;
}

ModuleStatusPtr VtkmModule::prepareInputGrid(const DataComponents &split, Object::const_ptr &grid,
                                             vtkm::cont::DataSet &dataset, const std::string &portName) const
{
    // read in grid
    grid = split.geometry;
    if (!grid) {
        std::ostringstream msg;
        msg << "No grid on input port " << (portName.empty() ? m_dataIn->getName() : portName) << "!";
        return Error(msg.str().c_str());
    }

    // transform to VTK-m
    return vtkmSetGrid(dataset, grid);
}

ModuleStatusPtr VtkmModule::prepareInputField(const DataComponents &split, DataBase::const_ptr &field,
                                              std::string &fieldName, vtkm::cont::DataSet &dataset,
                                              const std::string &portName) const
{
    // read in field
    field = split.mapped;
    if (!field) {
        std::stringstream msg;
        msg << "No mapped data on input port " << (portName.empty() ? m_dataIn->getName() : portName) << "!";
        return Error(msg.str().c_str());
    }

    // if the mapped data on the input port already has a name, keep it
    if (auto name = field->getAttribute("_species"); !name.empty())
        fieldName = name;

    // transform to VTK-m
    return vtkmAddField(dataset, field, fieldName);
}

ModuleStatusPtr VtkmModule::prepareInput(const std::shared_ptr<BlockTask> &task, Object::const_ptr &grid,
                                         DataBase::const_ptr &field, vtkm::cont::DataSet &dataset) const
{
    auto container = task->accept<Object>(m_dataIn);
    auto split = splitContainerObject(container);

    auto status = prepareInputGrid(split, grid, dataset);
    if (!isValid(status))
        return status;

    if (m_requireMappedData) {
        status = prepareInputField(split, field, m_fieldName, dataset);
        if (!isValid(status))
            return status;
    }

    return Success();
}

Object::ptr VtkmModule::prepareOutputGrid(vtkm::cont::DataSet &dataset) const
{
    Object::ptr geoOut = vtkmGetGeometry(dataset);
    if (!geoOut) {
        sendError("An error occurred while transforming the filter output grid to a Vistle object.");
        return nullptr;
    }

    updateMeta(geoOut);
    return geoOut;
}

DataBase::ptr VtkmModule::prepareOutputField(vtkm::cont::DataSet &dataset, const std::string &fieldName) const
{
    if (auto mapped = vtkmGetField(dataset, fieldName)) {
        std::cerr << "mapped data: " << *mapped << std::endl;
        updateMeta(mapped);
        return mapped;
    } else {
        sendError("An error occurred while transforming the filter output field %s to a Vistle object.",
                  fieldName.c_str());
        return nullptr;
    }
}

// Copies metadata attributes from the grid `from` to the grid `to`.
void copyMetadata(const Object::const_ptr &from, Object::ptr &to)
{
    to->copyAttributes(from);
    to->setTransform(from->getTransform());
    if (to->getTimestep() < 0) {
        to->setTimestep(from->getTimestep());
        to->setNumTimesteps(from->getNumTimesteps());
    }
    if (to->getBlock() < 0) {
        to->setBlock(from->getBlock());
        to->setNumBlocks(from->getNumBlocks());
    }
}

bool VtkmModule::prepareOutput(const std::shared_ptr<BlockTask> &task, vtkm::cont::DataSet &dataset,
                               Object::ptr &outputGrid, Object::const_ptr &inputGrid,
                               DataBase::const_ptr &inputField) const
{
    outputGrid = prepareOutputGrid(dataset);
    if (!outputGrid)
        return true;

    copyMetadata(inputGrid, outputGrid);

    if (m_requireMappedData) {
        auto mapped = prepareOutputField(dataset, m_fieldName);
        if (mapped) {
            mapped->copyAttributes(inputField);
            mapped->setGrid(outputGrid);
            task->addObject(m_dataOut, mapped);
            return true;
        }
    }

    // if there is no mapped data, add output grid to output port
    task->addObject(m_dataOut, outputGrid);
    return true;
}
