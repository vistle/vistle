#include <sstream>

#ifdef VERTTOCELL
#include <vtkm/filter/field_conversion/CellAverage.h>
#else
#include <vtkm/filter/field_conversion/PointAverage.h>
#endif

#include <vistle/vtkm/convert.h>

#include "CellToVertVtkm.h"

MODULE_MAIN(CellToVertVtkm)

using namespace vistle;

CellToVertVtkm::CellToVertVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm)
{
    m_inputPorts.push_back(m_dataIn);
    m_outputPorts.push_back(m_dataOut);

    for (int i = 0; i < NumPorts - 1; ++i) {
        std::string in("add_data_in");
        std::string out("add_data_out");
        if (i > 0) {
            in += std::to_string(i);
            out += std::to_string(i);
        }
        m_inputPorts.push_back(createInputPort(in, "additional input data"));
        m_outputPorts.push_back(createOutputPort(out, "converted data"));
    }
}

CellToVertVtkm::~CellToVertVtkm()
{}

ModuleStatusPtr CellToVertVtkm::ReadInPorts(const std::shared_ptr<BlockTask> &task, Object::const_ptr &grid,
                                            std::array<DataBase::const_ptr, NumPorts> &fields) const
{ // get grid and make sure all mapped data fields are defined on the same grid
    for (int i = 0; i < NumPorts; ++i) {
        auto container = task->accept<Object>(m_inputPorts[i]);
        auto split = splitContainerObject(container);
        auto data = split.mapped;

        // ... make sure there is data on the input port if the corresponding output port is connected
        if (!data && m_outputPorts[i]->isConnected()) {
            std::stringstream msg;
            msg << "No valid input data on " << m_inputPorts[i]->getName();
            return Error(msg.str().c_str());
        }

        // .. and add them to the vector
        fields[i] = data;

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

    if (!grid)
        return Error("Grid is required on at least one input object!");
    return Success();
}

ModuleStatusPtr CellToVertVtkm::transformInputField(DataBase::const_ptr &field, std::string &fieldName,
                                                    vtkm::cont::DataSet &dataset, const std::string &portName) const
{
    // read in field
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

ModuleStatusPtr CellToVertVtkm::prepareInputField(Object::const_ptr &grid, DataBase::const_ptr &field,
                                                  std::string &fieldName, vtkm::cont::DataSet &dataset) const
{
    assert(field);
    auto mapping = field->guessMapping(grid);
    // ... make sure the mapping is either vertex or element
    if (mapping != DataBase::Element && mapping != DataBase::Vertex) {
        std::stringstream msg;
        msg << "Unsupported data mapping " << field->mapping() << ", guessed=" << mapping << " on " << field->getName();
        return Error(msg.str().c_str());
    }
// ... and check if we actually need to apply the filter for the current port
#ifdef VERTTOCELL
    if (mapping == DataBase::Vertex) {
#else
    if (mapping == DataBase::Element) {
#endif
        // transform to VTK-m + add to dataset
        auto status = transformInputField(field, fieldName, dataset);
        if (!isValid(status))
            return status;
    }

    return Success();
}


void CellToVertVtkm::runFilter(vtkm::cont::DataSet &input, vtkm::cont::DataSet &output) const
{}

void CellToVertVtkm::runFilter(vtkm::cont::DataSet &input, std::string &fieldName, vtkm::cont::DataSet &output) const
{
    if (!input.HasField(fieldName))
        return;

#ifdef VERTTOCELL
    auto filter = vtkm::filter::field_conversion::CellAverage();
#else
    auto filter = vtkm::filter::field_conversion::PointAverage();
#endif
    filter.SetActiveField(fieldName);
    output = filter.Execute(input);
}

/*ModuleStatusPtr CheckField(const Object::const_ptr &grid, const DataBase::const_ptr &field, std::string& portName)
{
    auto status = VtkmModule2::CheckField(grid, field);
    if (!isValid(status)) {
        return status;
    }

    auto mapping = field->guessMapping(grid);
    // ... make sure the mapping is either vertex or element
    if (mapping != DataBase::Element && mapping != DataBase::Vertex) {
        std::stringstream msg;
        msg << "Unsupported data mapping " << field->mapping() << ", guessed=" << mapping << " on " << field->getName();
        return Error(msg.str().c_str());
    }
    return Success();
}*/

/*ModuleStatusPtr TransformField(const Object::const_ptr &grid, const DataBase::const_ptr &field, std::string &fieldName,
                               vtkm::cont::DataSet &dataset)
{
    auto mapping = field->guessMapping(grid);
    // ... and check if we actually need to apply the filter for the current port
#ifdef VERTTOCELL
    if (mapping == DataBase::Vertex) {
#else
    if (mapping == DataBase::Element) {
#endif
        // transform to VTK-m + add to dataset
        return VtkmModule2::TransformField(grid, field, fieldName, dataset);
    }

    return Success();
}*/

bool CellToVertVtkm::prepareOutput(vtkm::cont::DataSet &dataset, std::string &fieldName, Object::const_ptr &inputGrid,
                                   DataBase::const_ptr &inputField, Object::ptr &outputGrid,
                                   DataBase::ptr &outputField) const
{
    outputGrid = prepareOutputGrid(dataset);

    auto mapping = inputField->guessMapping(outputGrid);

// ... and check if we actually need to apply the filter for the current port
#ifdef VERTTOCELL
    if (mapping == DataBase::Vertex) {
#else
    if (mapping == DataBase::Element) {
#endif
        outputField = prepareOutputField(dataset, fieldName);
        if (outputField) {
            outputField->copyAttributes(inputField);
#ifdef VERTTOCELL
            outputField->setMapping(DataBase::Element);
#else
            outputField->setMapping(DataBase::Vertex);
#endif
            outputField->setGrid(outputGrid);
        }
    }

    return true;
}

/*bool prepareOutput(const std::shared_ptr<BlockTask> &task, Port *port, vtkm::cont::DataSet &dataset,
                   Object::ptr &outputGrid, Object::const_ptr &inputGrid, DataBase::const_ptr &inputField,
                   std::string &fieldName) const
{
    if (dataset.HasField(fieldName)) {
        VtkmModule2::prepareOutput(task, port, dataset, outputGrid, inputGrid, inputField, fieldName);
    } else {
        sendInfo("No filter applied for " + port->getName());
        auto ndata = inputField->clone();
        ndata->setGrid(inputGrid);
        updateMeta(ndata);
        task->addObject(port, ndata);
    }
}*/

bool CellToVertVtkm::compute(const std::shared_ptr<BlockTask> &task) const
{
    Object::const_ptr inputGrid;
    std::array<DataBase::const_ptr, NumPorts> inputFields;
    std::array<std::string, NumPorts> m_fieldNames;

    vtkm::cont::DataSet inputDataset, outputDataset;

    Object::ptr outputGrid;
    DataBase::ptr outputField;

    // read in grid and mapped data
    auto status = ReadInPorts(task, inputGrid, inputFields);
    if (!isValid(status))
        return true;

    // transform grid
    status = vtkmSetGrid(inputDataset, inputGrid);
    if (!isValid(status))
        return true;

    for (int i = 0; i < NumPorts; ++i) {
        // if the corresponding output port is connected...
        if (!m_outputPorts[i]->isConnected())
            continue;

        // ... check input field and transform it to VTK-m
        status = prepareInputField(inputGrid, inputFields[i], m_fieldNames[i], inputDataset);
        if (!isValid(status))
            return true;

        auto fieldName = m_fieldNames[i];
        if (inputDataset.HasField(fieldName)) {
            // ... run filter for each field ...
            runFilter(inputDataset, fieldName, outputDataset);

            // ... and transform filter output, i.e., grid and dataset, to Vistle objects
            prepareOutput(outputDataset, fieldName, inputGrid, inputFields[i], outputGrid, outputField);
            task->addObject(m_outputPorts[i], outputField);
        } else {
            // ... this is specific to the CellToVertVtkm module
            sendInfo("No filter applied for " + m_outputPorts[i]->getName());
            auto ndata = inputFields[i]->clone();
            ndata->setGrid(inputGrid);
            updateMeta(ndata);
            task->addObject(m_outputPorts[i], ndata);
        }
    }


    return true;
}
