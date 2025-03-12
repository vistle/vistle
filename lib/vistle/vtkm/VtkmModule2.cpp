#include <sstream>

#include <vistle/vtkm/convert.h>

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

ModuleStatusPtr VtkmModule2::CheckGrid(const Object::const_ptr &grid)
{
    if (!grid) {
        std::ostringstream msg;
        msg << "Could not find a valid input grid!";
        return Error(msg.str().c_str());
    }
    return Success();
}

ModuleStatusPtr VtkmModule2::TransformGrid(const Object::const_ptr &grid, vtkm::cont::DataSet &dataset) const
{
    return vtkmSetGrid(dataset, grid);
}

ModuleStatusPtr prepareInputGrid(const Object::const_ptr &grid, vtkm::cont::DataSet &dataset) const
{
    auto status = CheckGrid(grid);
    if (!isValid(status))
        return status;

    return TransformGrid(grid, dataset);
}

ModuleStatusPtr VtkmModule2::ReadInPorts(const std::shared_ptr<BlockTask> &task, Object::const_ptr &grid,
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
            status = vtkmSetGrid(inputDataset, inputGrid);
            if (!isValid(status))
                return true;
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

ModuleStatusPtr CheckField(const Object::const_ptr &grid, const DataBase::const_ptr &field, std::string &portName)
{
    if (!field) {
        std::stringstream msg;
        msg << "No mapped data on input port " << portName << "!";
        return Error(msg.str().c_str());
    }
    return Success();
}

ModuleStatusPtr TransformField(const Object::const_ptr &grid, const DataBase::const_ptr &field, std::string &fieldName,
                               vtkm::cont::DataSet &dataset)
{
    if (auto name = field->getAttribute("_species"); !name.empty())
        fieldName = name;

    return vtkmAddField(dataset, field, fieldName);
}

ModuleStatusPtr CellToVertVtkm::prepareInputField(const Object::const_ptr &grid, const DataBase::const_ptr &field,
                                                  std::string &fieldName, vtkm::cont::DataSet &dataset) const
{
    auto status = CheckField(grid, field);
    if (!isValid(status))
        return status;

    return TransformField(field, fieldName, dataset);
}

bool VtkmModule::prepareOutput(const std::shared_ptr<BlockTask> &task, Port *port, vtkm::cont::DataSet &dataset,
                               Object::ptr &outputGrid, Object::const_ptr &inputGrid, DataBase::const_ptr &inputField,
                               std::string &fieldName) const
{
    outputGrid = prepareOutputGrid(dataset, inputGrid, outputGrid);
    if (!outputGrid)
        return true;

    outputField = prepareOutputField(dataset, fieldName);

    if (outputField) {
        task->addObject(port, outputField);
    } else {
        task->addObject(port, outputGrid);
    }

    return true;
}

Object::ptr prepareOutputGrid(vtkm::cont::DataSet &dataset, Object::const_ptr &inputGrid, Object::ptr &outputGrid)
{
    auto outputGrid = vtkmGetGeometry(dataset);
    if (!outputGrid) {
        sendError("An error occurred while transforming the filter output grid to a Vistle object.");
        return nullptr;
    }

    updateMeta(outputGrid);
    copyMetadata(inputGrid, outputGrid);
    return outputGrid;
}

DataBase::ptr prepareOutputField(vtkm::cont::DataSet &dataset, DataBase::const_ptr &inputField, std::string &fieldName,
                                 Object::const_ptr &inputGrid, Object::ptr &outputGrid) const
{
    if (m_requireMappedData) {
        if (auto mapped = vtkmGetField(dataset, fieldName);) {
            std::cerr << "mapped data: " << *mapped << std::endl;
            updateMeta(mapped);
            mapped->copyAttributes(inputField);
            mapped->setGrid(outputGrid);
            return mapped;
        } else {
            sendError("An error occurred while transforming the filter output field %s to a Vistle object.",
                      fieldName.c_str());
            return nullptr;
        }
    }
}

bool VtkmModule2::compute(const std::shared_ptr<BlockTask> &task) const
{
    Object::const_ptr inputGrid;
    std::vector<DataBase::const_ptr, m_numPorts> inputFields;

    vtkm::cont::DataSet inputDataset, outputDataset;

    auto status = ReadInPorts(task, inputGrid, inputFields);
    if (!isValid(status))
        return true;
    // TODO: make this status
    assert(m_outputPorts.size() == inputFields.size());

    status = prepareInputGrid(inputGrid, inputDataset);
    if (!isValid(status))
        return true;

    for (auto i = 0; i < inputFields.size(); ++i) {
        Object::ptr outputGrid;
        DataBase::ptr outputField;
        std::string fieldName = "";

        // if the corresponding output port is connected...
        if (!m_outputPorts[i]->isConnected())
            continue;

        // ... check input field and transform it to VTK-m
        status = prepareInputField(inputGrid, inputFields[i], fieldName, inputDataset);
        if (!isValid(status))
            return true;

        // ... run filter for each field ...
        runFilter(inputDataset, fieldName, outputDataset);

        // ... and transform filter output, i.e., grid and dataset, to Vistle objects
        prepareOutput(outputDataset, m_outputPorts[i], fieldName, inputGrid, inputFields[i], outputGrid, outputField);
    }


    return true;
}

bool VtkmModule2::isValid(const ModuleStatusPtr &status) const
{
    if (strcmp(status->message(), ""))
        sendText(status->messageType(), status->message());

    return status->continueExecution();
}
