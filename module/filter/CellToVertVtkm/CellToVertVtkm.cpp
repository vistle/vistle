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

ModuleStatusPtr CellToVertVtkm::GetCommonGrid(const std::shared_ptr<vistle::BlockTask> &task,
                                              vistle::Object::const_ptr &inputGrid,
                                              std::array<vistle::DataComponents, NumPorts> &m_splits) const
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
        m_splits[i] = split;

        if (!data)
            continue;

        // ... make sure all data fields are defined on the same grid
        if (inputGrid) {
            if (split.geometry && split.geometry->getHandle() != inputGrid->getHandle()) {
                std::stringstream msg;
                msg << "The grid on " << m_inputPorts[i]->getName()
                    << " does not match the grid on the other input ports!";
                return Error(msg.str().c_str());
            }
        } else {
            inputGrid = split.geometry;
        }
    }

    if (!inputGrid)
        return Error("Grid is required on at least one input object!");
    return Success();
}

ModuleStatusPtr CellToVertVtkm::prepareInput(vistle::Object::const_ptr &inputGrid,
                                             vistle::DataBase::const_ptr &inputField, vtkm::cont::DataSet &input,
                                             vistle::DataComponents &split, std::string &fieldName) const
{
    auto data = split.mapped;
    assert(data);
    auto mapping = data->guessMapping(inputGrid);
    // ... make sure the mapping is either vertex or element
    if (mapping != DataBase::Element && mapping != DataBase::Vertex) {
        std::stringstream msg;
        msg << "Unsupported data mapping " << data->mapping() << ", guessed=" << mapping << " on " << data->getName();
        return Error(msg.str().c_str());
    }
// ... and check if we actually need to apply the filter for the current port
#ifdef VERTTOCELL
    if (mapping == DataBase::Vertex) {
#else
    if (mapping == DataBase::Element) {
#endif
        // transform to VTK-m + add to dataset
        auto status = prepareInputField(split, data, fieldName, input);
        if (!isValid(status))
            return status;
    }

    return Success();
}


void CellToVertVtkm::runFilter(vtkm::cont::DataSet &input, vtkm::cont::DataSet &output) const
{}

void CellToVertVtkm::runFilter(vtkm::cont::DataSet &input, vtkm::cont::DataSet &output, std::string &fieldName) const
{
#ifdef VERTTOCELL
    auto filter = vtkm::filter::field_conversion::CellAverage();
#else
    auto filter = vtkm::filter::field_conversion::PointAverage();
#endif
    filter.SetActiveField(fieldName);
    output = filter.Execute(input);
}

bool CellToVertVtkm::prepareOutput(vtkm::cont::DataSet &dataset, vistle::Object::ptr &outputGrid,
                                   vistle::DataBase::ptr &outputField, vistle::Object::const_ptr &inputGrid,
                                   vistle::DataBase::const_ptr &inputField, vistle::DataComponents &split,
                                   std::string &fieldName) const
{
    outputGrid = prepareOutputGrid(dataset);

    auto data = split.mapped;
    auto mapping = data->guessMapping(inputGrid);

// ... and check if we actually need to apply the filter for the current port
#ifdef VERTTOCELL
    if (mapping == DataBase::Vertex) {
#else
    if (mapping == DataBase::Element) {
#endif
        outputField = prepareOutputField(dataset, fieldName);
        if (outputField) {
            outputField->copyAttributes(data);
#ifdef VERTTOCELL
            outputField->setMapping(DataBase::Element);
#else
            outputField->setMapping(DataBase::Vertex);
#endif
            outputField->setGrid(inputGrid);
        }
    }

    return true;
}

bool CellToVertVtkm::compute(const std::shared_ptr<BlockTask> &task) const
{
    Object::const_ptr inputGrid;
    Object::ptr outputGrid;
    DataBase::const_ptr inputField;
    DataBase::ptr outputField;

    vtkm::cont::DataSet input, output;

    std::array<vistle::DataComponents, NumPorts> m_splits;

    // prepare grid
    auto status = GetCommonGrid(task, inputGrid, m_splits);
    if (!isValid(status))
        return true;

    // transform grid
    status = vtkmSetGrid(input, inputGrid);
    if (!isValid(status))
        return true;

    // prepare fields
    for (int i = 0; i < NumPorts; ++i) {
        // if the corresponding output port is connected...
        if (!m_outputPorts[i]->isConnected())
            continue;
        std::string fieldNameI;
        status = prepareInput(inputGrid, inputField, input, m_splits[i], fieldNameI);
        if (!isValid(status))
            return true;

        if (input.HasField(fieldNameI)) {
            runFilter(input, output, fieldNameI);

            prepareOutput(output, outputGrid, outputField, inputGrid, inputField, m_splits[i], fieldNameI);
            task->addObject(m_outputPorts[i], outputField);
        } else {
            sendInfo("No filter applied for " + m_outputPorts[i]->getName());
            auto ndata = m_splits[i].mapped->clone();
            ndata->setGrid(outputGrid);
            updateMeta(ndata);
            task->addObject(m_outputPorts[i], ndata);
        }
    }


    return true;
}
