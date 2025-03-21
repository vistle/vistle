#include <sstream>

#include <vistle/vtkm/convert.h>

#include "convert.h"

#include "VtkmModule.h"

using namespace vistle;

namespace {
std::string getFieldName(int i, bool output = false)
{
    /*
    By default, VTK-m names output fields the same as input fields which causes problems
    if the input mapping is different from the output mapping, i.e., when converting 
    a point field to a cell field or vice versa. To avoid having a point and a 
    cell field of the same name in the resulting dataset, which leads to conflicts, e.g., 
    when calling VTK-m's GetField() method, we rename the output field here.
    */
    std::string name = "data_at_port_" + std::to_string(i);
    if (i == 0 && output)
        name += "_out";
    return name;
}
} // namespace


VtkmModule::VtkmModule(const std::string &name, int moduleID, mpi::communicator comm, int numPorts,
                       bool requireMappedData)
: Module(name, moduleID, comm), m_numPorts(numPorts), m_requireMappedData(requireMappedData)
{
    assert(m_numPorts > 0);

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

bool VtkmModule::prepare()
{
    if (!m_inputPorts[0]->isConnected()) {
        if (rank() == 0)
            sendError("No input connected to %s", m_inputPorts[0]->getName().c_str());
        return false;
    }

    for (int i = 0; i < m_numPorts; ++i) {
        if (!m_inputPorts[i]->isConnected() && m_outputPorts[i]->isConnected()) {
            std::stringstream msg;
            msg << "Output port " << m_outputPorts[i]->getName() << " is connected, but corresponding input port "
                << m_inputPorts[i]->getName() << " is not";
            if (rank() == 0)
                sendError(msg.str());
            return false;
        }
    }

    return Module::prepare();
}

ModuleStatusPtr VtkmModule::readInPorts(const std::shared_ptr<BlockTask> &task, Object::const_ptr &grid,
                                        std::vector<DataBase::const_ptr> &fields) const
{
    for (int i = 0; i < m_numPorts; ++i) {
        if (!m_inputPorts[i]->isConnected()) {
            fields.push_back(nullptr);
            continue;
        }

        auto container = task->accept<Object>(m_inputPorts[i]);
        auto split = splitContainerObject(container);
        auto geometry = split.geometry;
        auto data = split.mapped;

        // ... make sure there is data on the input port if the corresponding output port is connected
        if (!geometry && !data) {
            std::stringstream msg;
            msg << "No data on input port " << m_inputPorts[i]->getName() << ", even though it is connected";
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

    if (!grid) {
        std::ostringstream msg;
        msg << "Could not find a valid input grid on any input port";
        return Error(msg.str());
    }

    return Success();
}

ModuleStatusPtr VtkmModule::prepareInputGrid(const Object::const_ptr &grid, vtkm::cont::DataSet &dataset) const
{
    return vtkmSetGrid(dataset, grid);
}

ModuleStatusPtr VtkmModule::prepareInputField(const Port *port, const Object::const_ptr &grid,
                                              const DataBase::const_ptr &field, std::string &fieldName,
                                              vtkm::cont::DataSet &dataset) const
{
    return vtkmAddField(dataset, field, fieldName);
}

Object::ptr VtkmModule::prepareOutputGrid(const vtkm::cont::DataSet &dataset, const Object::const_ptr &inputGrid) const
{
    auto outputGrid = vtkmGetGeometry(dataset);
    if (!outputGrid) {
        sendError("An error occurred while transforming the filter output grid to a Vistle object.");
        return nullptr;
    }

    updateMeta(outputGrid);
    outputGrid->copyAttributes(inputGrid);
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

    status = prepareInputGrid(inputGrid, inputDataset);
    if (!isValid(status))
        return true;

    for (std::size_t i = 0; i < inputFields.size(); ++i) {
        fieldNames.push_back(getFieldName(i));

        // if the corresponding output port is connected...
        if (i > 0 && !m_outputPorts[i]->isConnected())
            continue;

        // ... check input field and transform it to VTK-m ...
        if (m_requireMappedData) {
            if (!inputFields[i]) {
                std::stringstream msg;
                msg << "No mapped data on input port " << m_inputPorts[i]->getName();
                status = Error(msg.str());
                if (!isValid(status))
                    return true;
            }
        }
        if (inputFields[i]) {
            status = prepareInputField(m_inputPorts[i], inputGrid, inputFields[i], fieldNames[i], inputDataset);
            if (!isValid(status))
                return true;
        }
    }

    // ... run filter on the active field ...
    auto activeField = fieldNames[0];
    if (!m_requireMappedData || inputDataset.HasField(activeField)) {
        auto filter = setUpFilter();
        if (inputDataset.HasField(activeField)) {
            filter->SetActiveField(activeField);
            filter->SetOutputFieldName(getFieldName(0, true));
        }
        outputDataset = filter->Execute(inputDataset);
    }

    // ... transform filter output, i.e., grid and data fields, to Vistle objects
    auto outputGrid = prepareOutputGrid(outputDataset, inputGrid);
    for (std::size_t i = 0; i < inputFields.size(); ++i) {
        DataBase::ptr outputField;
        if (!m_outputPorts[i]->isConnected())
            continue;

        if (inputFields[i]) {
            std::string name = fieldNames[i];
            if (i == 0 && outputDataset.HasField(getFieldName(i, true))) {
                // if filter has created a dedicated output field, use it
                name = getFieldName(i, true);
            }
            outputField = prepareOutputField(outputDataset, inputGrid, inputFields[i], name, outputGrid);
        }

        // ... and write the result to the output ports
        if (outputField) {
            task->addObject(m_outputPorts[i], outputField);
        } else if (outputGrid) {
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
