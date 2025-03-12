#include <sstream>

#include <vistle/vtkm/convert.h>

#include "convert.h"

#include "VtkmModule2.h"

using namespace vistle;

VtkmModule2::VtkmModule2(const std::string &name, int moduleID, mpi::communicator comm, int numPorts,
                         bool requireMappedData)
: Module(name, moduleID, comm), m_numPorts(numPorts), m_requireMappedData(requireMappedData)
{
    for (int i = 0; i < m_numPorts; ++i) {
        std::string in("data_in");
        std::string out("data_out");
        if (i > 0) {
            in += std::to_string(i);
            out += std::to_string(i);
        }
        m_inputPorts.push_back(createInputPort(in, m_requireMappedData ? "input grid with mapped data" : "input grid"));
        m_outputPorts.push_back(
            createOutputPort(out, m_requireMappedData ? "output grid with mapped data" : "output grid"));
    }
}

VtkmModule2::~VtkmModule2()
{}

// Copies metadata attributes from the grid `from` to the grid `to`.
void copyMetadata2(const Object::const_ptr &from, Object::ptr &to)
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

ModuleStatusPtr VtkmModule2::checkInputGrid(const Object::const_ptr &grid) const
{
    if (!grid) {
        std::ostringstream msg;
        msg << "Could not find a valid input grid!";
        return Error(msg.str().c_str());
    }
    return Success();
}

ModuleStatusPtr VtkmModule2::transformInputGrid(const Object::const_ptr &grid, vtkm::cont::DataSet &dataset) const
{
    return vtkmSetGrid(dataset, grid);
}

ModuleStatusPtr VtkmModule2::readInPorts(const std::shared_ptr<BlockTask> &task, Object::const_ptr &grid,
                                         std::vector<DataBase::const_ptr> &fields) const
{ // get grid and make sure all mapped data fields are defined on the same grid
    for (int i = 0; i < m_numPorts; ++i) {
        auto container = task->accept<Object>(m_inputPorts[i]);
        auto split = splitContainerObject(container);
        auto data = split.mapped;

        // ... make sure there is data on the input port if the corresponding output port is connected
        if (!data && m_outputPorts[i]->isConnected()) {
            std::stringstream msg;
            msg << "No mapped data on input port " << m_inputPorts[i]->getName() << "!";
            return Error(msg.str().c_str());
        }

        // .. and add them to the vector
        fields.push_back(data);

        if (!data)
            continue;

        // ... make sure all data fields are defined on the same grid
        if (grid) {
            if (split.geometry && split.geometry->getHandle() != grid->getHandle()) {
                std::stringstream msg;
                msg << "The grid on " << m_inputPorts[i]->getName()
                    << " does not match the grid on the other input ports!";
                return Error(msg.str().c_str());
            }
        } else {
            grid = split.geometry;
        }
    }

    return Success();
}

ModuleStatusPtr VtkmModule2::checkInputField(const Object::const_ptr &grid, const DataBase::const_ptr &field,
                                             const std::string &portName) const
{
    if (!field) {
        std::stringstream msg;
        msg << "No mapped data on input port " << portName << "!";
        return Error(msg.str().c_str());
    }
    return Success();
}

ModuleStatusPtr VtkmModule2::transformInputField(const Object::const_ptr &grid, const DataBase::const_ptr &field,
                                                 std::string &fieldName, vtkm::cont::DataSet &dataset) const
{
    if (auto name = field->getAttribute("_species"); !name.empty())
        fieldName = name;

    return vtkmAddField(dataset, field, fieldName);
}

bool VtkmModule2::prepareOutput(const std::shared_ptr<vistle::BlockTask> &task, vistle::Port *port,
                                vtkm::cont::DataSet &dataset, vistle::Object::ptr &outputGrid,
                                vistle::Object::const_ptr &inputGrid, vistle::DataBase::const_ptr &inputField,
                                std::string &fieldName, DataBase::ptr &outputField) const
{
    outputGrid = prepareOutputGrid(dataset, inputGrid, outputGrid);
    if (!outputGrid)
        return true;

    outputField = prepareOutputField(dataset, inputField, fieldName, inputGrid, outputGrid);

    //TODO: make this its own function
    if (outputField) {
        task->addObject(port, outputField);
    } else {
        task->addObject(port, outputGrid);
    }

    return true;
}

Object::ptr VtkmModule2::prepareOutputGrid(vtkm::cont::DataSet &dataset, const Object::const_ptr &inputGrid,
                                           Object::ptr &outputGrid) const
{
    outputGrid = vtkmGetGeometry(dataset);
    if (!outputGrid) {
        sendError("An error occurred while transforming the filter output grid to a Vistle object.");
        return nullptr;
    }

    updateMeta(outputGrid);
    copyMetadata2(inputGrid, outputGrid);
    return outputGrid;
}

DataBase::ptr VtkmModule2::prepareOutputField(vtkm::cont::DataSet &dataset, const DataBase::const_ptr &inputField,
                                              std::string &fieldName, const Object::const_ptr &inputGrid,
                                              Object::ptr &outputGrid) const
{
    if (m_requireMappedData) {
        if (auto mapped = vtkmGetField(dataset, fieldName)) {
            std::cerr << "mapped data: " << *mapped << std::endl;
            updateMeta(mapped);
            mapped->copyAttributes(inputField);
            mapped->setGrid(outputGrid);
            return mapped;
        } else {
            sendError("An error occurred while transforming the filter output field %s to a Vistle object.",
                      fieldName.c_str());
        }
    }
    return nullptr;
}

bool VtkmModule2::compute(const std::shared_ptr<BlockTask> &task) const
{
    Object::const_ptr inputGrid;
    std::vector<DataBase::const_ptr> inputFields;

    vtkm::cont::DataSet inputDataset, outputDataset;

    auto status = readInPorts(task, inputGrid, inputFields);
    if (!isValid(status))
        return true;
    // TODO: make this status
    assert(m_outputPorts.size() == inputFields.size());

    status = checkInputGrid(inputGrid);
    if (!isValid(status))
        return true;

    status = transformInputGrid(inputGrid, inputDataset);
    if (!isValid(status))
        return true;

    for (std::size_t i = 0; i < inputFields.size(); ++i) {
        Object::ptr outputGrid;
        DataBase::ptr outputField;
        std::string fieldName = "";

        // if the corresponding output port is connected...
        if (!m_outputPorts[i]->isConnected())
            continue;

        // ... check input field and transform it to VTK-m
        status = checkInputField(inputGrid, inputFields[i], m_inputPorts[i]->getName());
        if (!isValid(status))
            return true;

        status = transformInputField(inputGrid, inputFields[i], fieldName, inputDataset);
        if (!isValid(status))
            return true;

        // ... run filter for each field ...
        runFilter(inputDataset, fieldName, outputDataset);

        // ... and transform filter output, i.e., grid and dataset, to Vistle objects
        prepareOutput(task, m_outputPorts[i], outputDataset, outputGrid, inputGrid, inputFields[i], fieldName,
                      outputField);
    }


    return true;
}

bool VtkmModule2::isValid(const ModuleStatusPtr &status) const
{
    if (strcmp(status->message(), ""))
        sendText(status->messageType(), status->message());

    return status->continueExecution();
}
