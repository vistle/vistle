#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/triangles.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>
#include <vistle/alg/objalg.h>
#include <vistle/vtkm/convert.h>

#include "CellToVertVtkm.h"

#ifdef VERTTOCELL
#include <vtkm/filter/field_conversion/CellAverage.h>
#else
#include <vtkm/filter/field_conversion/PointAverage.h>
#endif

using namespace vistle;

MODULE_MAIN(CellToVertVtkm)

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

ModuleStatusPtr CellToVertVtkm::prepareInput(const std::shared_ptr<vistle::BlockTask> &task,
                                             vistle::Object::const_ptr &grid, vistle::DataBase::const_ptr &field,
                                             vtkm::cont::DataSet &dataset) const
{
    // iterate through all input ports!
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

    // transform grid
    auto status = vtkmSetGrid(dataset, grid);
    if (!isValid(status))
        return status;

    for (int i = 0; i < NumPorts; ++i) {
        // if the corresponding output port is connected...
        if (!m_outputPorts[i]->isConnected())
            continue;

        auto data = m_splits[i].mapped;
        auto mapping = data->guessMapping(grid);
        // ... make sure the mapping is either vertex or element
        if (mapping != DataBase::Element && mapping != DataBase::Vertex) {
            std::stringstream msg;
            msg << "Unsupported data mapping " << data->mapping() << ", guessed=" << mapping << " on "
                << data->getName();
            return Error(msg.str().c_str());
        }
// ... and check if we actually need to apply the filter for the current port
#ifdef VERTTOCELL
        if (mapping == DataBase::Element) {
#else
        if (mapping == DataBase::Vertex) {
#endif
            // if not, just forward the data
            auto ndata = m_splits[i].mapped->clone();
            // TODO: Why?
            ndata->setMapping(DataBase::Vertex);
            ndata->setGrid(grid);
            updateMeta(ndata);
            task->addObject(m_outputPorts[i], ndata);

        } else {
            //     transform to VTK-m + add to dataset

            //BUG: this shouldn't be field all the time...
            auto status = prepareInputField(m_splits[i], field, m_fieldNames[i], dataset);
            if (!isValid(status))
                return status;
            m_transformIndices.push_back(i);
        }
    }
    return Success();
}

bool CellToVertVtkm::prepareOutput(const std::shared_ptr<vistle::BlockTask> &task, vtkm::cont::DataSet &dataset,
                                   vistle::Object::ptr &outputGrid, vistle::Object::const_ptr &inputGrid,
                                   vistle::DataBase::const_ptr &inputField) const
{
    if (m_transformIndices.empty())
        return true;

    outputGrid = prepareOutputGrid(dataset);

    // iterate through all input ports
    for (const auto &i: m_transformIndices) {
        auto outputField = prepareOutputField(dataset, m_fieldNames[i]);
        outputField->copyAttributes(m_splits[i].mapped);
#ifdef VERTTOCELL
        outputField->setMapping(DataBase::Element);
#else
        outputField->setMapping(DataBase::Vertex);
#endif
        outputField->setGrid(outputGrid);
        task->addObject(m_outputPorts[i], outputField);
    }
    return true;
}

void CellToVertVtkm::runFilter(vtkm::cont::DataSet &input, vtkm::cont::DataSet &output) const
{
    if (m_transformIndices.empty())
        return;
#ifdef VERTTOCELL
    auto filter = vtkm::filter::field_conversion::CellAverage();
#else
    auto filter = vtkm::filter::field_conversion::PointAverage();
#endif
    filter.SetActiveField(m_fieldNames[m_transformIndices[0]]);
    output = filter.Execute(input);
}


/*bool CellToVertVtkm::compute(const std::shared_ptr<BlockTask> &task) const
{
    Object::const_ptr grid;
    std::vector<DataBase::const_ptr> data_vec;

    for (int i = 0; i < NumAddPorts; ++i) {
        auto &data_in = m_inputPorts[i];
        auto container = task->accept<Object>(data_in);
        auto split = splitContainerObject(container);
        auto data = split.mapped;
        // check which input ports are connected
        if (!data && m_outputPorts[i]->isConnected()) {
            sendError("no valid input data on %s", data_in->getName().c_str());
            return true;
        }

        // and add them to the vector
        data_vec.emplace_back(data);
        if (!data)
            continue;

        if (grid) {
            // make sure all data fields are defined on the same grid!
            if (split.geometry && split.geometry->getHandle() != grid->getHandle()) {
                sendError("grids have to match on all input objects");
                return true;
            }
        } else {
            grid = split.geometry;
        }
    }

    assert(m_outputPorts.size() == data_vec.size());

    if (!grid) {
        sendError("grid is required on at least one input object");
        return true;
    }

    for (int i = 0; i < NumAddPorts; ++i) {
        // only do something if the output port is connected
        auto &data_out = m_outputPorts[i];
        if (!data_out->isConnected())
            continue;

        auto &data = data_vec[i];
        assert(data);
        auto mapping = data->guessMapping(grid);
        // make sure mapping is either vertex or element
        if (mapping != DataBase::Element && mapping != DataBase::Vertex) {
            std::stringstream str;
            str << "unsupported data mapping " << data->mapping() << ", guessed=" << mapping << " on "
                << data->getName();
            std::string s = str.str();
            sendError("%s", s.c_str());
            return true;
        }
    }

    for (int i = 0; i < NumAddPorts; ++i) {
        auto &data_out = m_outputPorts[i];
        // do nothing if the output port is not connected
        if (!data_out->isConnected())
            continue;

        auto &data = data_vec[i];
        assert(data);
        // get the mapping of the input grid
        auto mapping = data->guessMapping(grid);
        // only apply filter if it makes sense, otherwise just forward the data
#ifdef VERTTOCELL
        if (mapping == DataBase::Element) {
#else
        if (mapping == DataBase::Vertex) {
#endif
            auto ndata = data->clone();
            ndata->setMapping(DataBase::Vertex);
            ndata->setGrid(grid);
            updateMeta(ndata);
            task->addObject(data_out, ndata);
        } else {
            // transform vistle dataset to vtkm dataset
            vtkm::cont::DataSet vtkmDataSet;
            auto status = vtkmSetGrid(vtkmDataSet, grid);
            if (!status->continueExecution()) {
                sendText(status->messageType(), status->message());
                return true;
            }

            // apply vtkm clip filter
#ifdef VERTTOCELL
            auto filter = vtkm::filter::field_conversion::CellAverage();
#else
            auto filter = vtkm::filter::field_conversion::PointAverage();
#endif
            std::string mapSpecies;
            mapSpecies = data->getAttribute("_species");
            if (mapSpecies.empty())
                mapSpecies = "mapdata";
            status = vtkmAddField(vtkmDataSet, data, mapSpecies);
            if (!status->continueExecution()) {
                sendText(status->messageType(), status->message());
                return true;
            }

            filter.SetActiveField(mapSpecies);
            auto avg = filter.Execute(vtkmDataSet);

            // transform result back into vistle format
            if (auto mapped = vtkmGetField(avg, mapSpecies)) {
                std::cerr << "mapped data: " << *mapped << std::endl;
                mapped->copyAttributes(data);
                // set correct mapping
#ifdef VERTTOCELL
                mapped->setMapping(DataBase::Element);
#else
                mapped->setMapping(DataBase::Vertex);
#endif
                mapped->setGrid(grid);
                updateMeta(mapped);
                // and add to output port
                task->addObject(data_out, mapped);
                return true;
            } else {
                sendError("could not handle mapped data");
                return true;
            }
        }
    }

    return true;
}*/
