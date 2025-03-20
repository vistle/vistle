#include <sstream>

#include <vistle/vtkm/convert.h>

#include "convert.h"

#include "VtkmModule.h"

using namespace vistle;

VtkmModule::VtkmModule(const std::string &name, int moduleID, mpi::communicator comm, int numPorts,
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

VtkmModule::~VtkmModule()
{}

ModuleStatusPtr VtkmModule::readInPorts(const std::shared_ptr<BlockTask> &task, Object::const_ptr &grid,
                                        std::vector<DataBase::const_ptr> &fields) const
{
    for (int i = 0; i < m_numPorts; ++i) {
        if (!m_outputPorts[i]->isConnected()) {
            continue;
        }

        auto container = task->accept<Object>(m_inputPorts[i]);
        auto split = splitContainerObject(container);
        auto geometry = split.geometry;
        auto data = split.mapped;

        // ... make sure there is data on the input port if the corresponding output port is connected
        if (!geometry && !data) {
            std::stringstream msg;
            msg << "No data on input port " << m_inputPorts[i]->getName() << ", even though "
                << m_outputPorts[i]->getName() << "is connected!";
            return Error(msg.str());
        }

        // .. and add it to the vector
        fields.push_back(data);

        // ... make sure all data fields are defined on the same grid
        if (grid) {
            if (geometry && geometry->getHandle() != grid->getHandle()) {
                std::stringstream msg;
                msg << "The grid on " << m_inputPorts[i]->getName()
                    << " does not match the grid on the other input ports!";
                return Error(msg.str());
            }
        } else {
            grid = geometry;
        }
    }

    return Success();
}

ModuleStatusPtr VtkmModule::checkInputGrid(const Object::const_ptr &grid) const
{
    if (!grid) {
        std::ostringstream msg;
        msg << "Could not find a valid input grid!";
        return Error(msg.str());
    }
    return Success();
}

ModuleStatusPtr VtkmModule::transformInputGrid(const Object::const_ptr &grid, vtkm::cont::DataSet &dataset) const
{
    return vtkmSetGrid(dataset, grid);
}

ModuleStatusPtr VtkmModule::checkInputField(const Object::const_ptr &grid, const DataBase::const_ptr &field,
                                            const std::string &portName) const
{
    if (!field) {
        std::stringstream msg;
        msg << "No mapped data on input port " << portName << "!";
        return Error(msg.str());
    }
    return Success();
}

ModuleStatusPtr VtkmModule::transformInputField(const Object::const_ptr &grid, const DataBase::const_ptr &field,
                                                std::string &fieldName, vtkm::cont::DataSet &dataset) const
{
    if (auto name = field->getAttribute("_species"); !name.empty())
        fieldName = name;

    return vtkmAddField(dataset, field, fieldName);
}

// Copies metadata attributes from the grid `from` to the grid `to`.
void copyMetadata(const Object::const_ptr &from, Object::ptr &to)
{
    to->copyAttributes(from);
}

Object::ptr VtkmModule::prepareOutputGrid(const vtkm::cont::DataSet &dataset, const Object::const_ptr &inputGrid) const
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

DataBase::ptr VtkmModule::prepareOutputField(const vtkm::cont::DataSet &dataset, const Object::const_ptr &inputGrid,
                                             const DataBase::const_ptr &inputField, const std::string &fieldName,
                                             const Object::ptr &outputGrid) const
{
    if (auto mapped = vtkmGetField(dataset, fieldName)) {
        std::cerr << "mapped data: " << *mapped << std::endl;
        updateMeta(mapped);
        mapped->copyAttributes(inputField);
        if (outputGrid)
            mapped->setGrid(outputGrid);
        return mapped;
    } else {
        sendError("An error occurred while transforming the filter output field %s to a Vistle object.",
                  fieldName.c_str());
    }

    return nullptr;
}

bool VtkmModule::compute(const std::shared_ptr<BlockTask> &task) const
{
    Object::const_ptr inputGrid;
    std::vector<DataBase::const_ptr> inputFields;
    std::vector<std::string> fieldNames;

    vtkm::cont::DataSet inputDataset, outputDataset;

    auto status = readInPorts(task, inputGrid, inputFields);
    if (!isValid(status))
        return true;

    assert(m_outputPorts.size() == inputFields.size());

    status = checkInputGrid(inputGrid);
    if (!isValid(status))
        return true;

    status = transformInputGrid(inputGrid, inputDataset);
    if (!isValid(status))
        return true;

    std::string activeField;
    for (std::size_t i = 0; i < inputFields.size(); ++i) {
        fieldNames.push_back("data_at_port_" + std::to_string(i));

        // if the corresponding output port is connected...
        if (!m_outputPorts[i]->isConnected())
            continue;

        // ... check input field and transform it to VTK-m ...
        if (m_requireMappedData) {
            status = checkInputField(inputGrid, inputFields[i], m_inputPorts[i]->getName());
            if (!isValid(status))
                return true;

            status = transformInputField(inputGrid, inputFields[i], fieldNames[i], inputDataset);
            if (!isValid(status))
                return true;
        }
        if (activeField.empty())
            activeField = fieldNames[i];
    }

    // ... run filter on the active field ...
    runFilter(inputDataset, activeField, outputDataset);

    // ... transform filter output, i.e., grid and data fields, to Vistle objects
    auto outputGrid = prepareOutputGrid(outputDataset, inputGrid);
    for (std::size_t i = 0; i < inputFields.size(); ++i) {
        DataBase::ptr outputField;
        if (!m_outputPorts[i]->isConnected())
            continue;

        if (m_requireMappedData)
            outputField = prepareOutputField(outputDataset, inputGrid, inputFields[i], fieldNames[i], outputGrid);

        // ... and write the result to the output ports
        if (outputField) {
            task->addObject(m_outputPorts[i], outputField);
        } else {
            task->addObject(m_outputPorts[i], outputGrid);
        }
    }


    return true;
}

bool VtkmModule::isValid(const ModuleStatusPtr &status) const
{
    if (strcmp(status->message(), ""))
        sendText(status->messageType(), status->message());

    return status->continueExecution();
}
