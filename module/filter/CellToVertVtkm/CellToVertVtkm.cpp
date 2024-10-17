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
: Module(name, moduleID, comm)
{
    for (int i = 0; i < NumPorts; ++i) {
        std::string in("data_in");
        std::string out("data_out");
        if (i > 0) {
            in += std::to_string(i);
            out += std::to_string(i);
        }
        m_data_in.push_back(createInputPort(in, "input data"));
        m_data_out.push_back(createOutputPort(out, "converted data"));
    }
}

CellToVertVtkm::~CellToVertVtkm()
{}

bool CellToVertVtkm::compute(const std::shared_ptr<BlockTask> &task) const
{
    Object::const_ptr grid;
    std::vector<DataBase::const_ptr> data_vec;

    for (int i = 0; i < NumPorts; ++i) {
        auto &data_in = m_data_in[i];
        auto container = task->accept<Object>(data_in);
        auto split = splitContainerObject(container);
        auto data = split.mapped;
        if (!data && m_data_out[i]->isConnected()) {
            sendError("no valid input data on %s", data_in->getName().c_str());
            return true;
        }

        data_vec.emplace_back(data);
        if (!data)
            continue;

        if (grid) {
            if (split.geometry && split.geometry->getHandle() != grid->getHandle()) {
                sendError("grids have to match on all input objects");
                return true;
            }
        } else {
            grid = split.geometry;
        }
    }

    assert(m_data_out.size() == data_vec.size());

    if (!grid) {
        sendError("grid is required on at least one input object");
        return true;
    }

    for (int i = 0; i < NumPorts; ++i) {
        auto &data_out = m_data_out[i];
        if (!data_out->isConnected())
            continue;

        auto &data = data_vec[i];
        assert(data);
        auto mapping = data->guessMapping(grid);
        if (mapping != DataBase::Element && mapping != DataBase::Vertex) {
            std::stringstream str;
            str << "unsupported data mapping " << data->mapping() << ", guessed=" << mapping << " on "
                << data->getName();
            std::string s = str.str();
            sendError("%s", s.c_str());
            return true;
        }
    }

    for (int i = 0; i < NumPorts; ++i) {
        auto &data_out = m_data_out[i];
        if (!data_out->isConnected())
            continue;

        auto &data = data_vec[i];
        assert(data);
        auto mapping = data->guessMapping(grid);
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
            if (status == VtkmTransformStatus::UNSUPPORTED_GRID_TYPE) {
                sendError("Currently only supporting unstructured grids");
                return true;
            } else if (status == VtkmTransformStatus::UNSUPPORTED_CELL_TYPE) {
                sendError(
                    "Can only transform these cells from vistle to vtkm: point, bar, triangle, polygon, quad, tetra, "
                    "hexahedron, pyramid");
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
            if (status == VtkmTransformStatus::UNSUPPORTED_FIELD_TYPE) {
                sendError("Unsupported mapped field type");
                return true;
            }
            filter.SetActiveField(mapSpecies);
            auto avg = filter.Execute(vtkmDataSet);

            // transform result back into vistle format
            if (auto mapped = vtkmGetField(avg, mapSpecies)) {
                std::cerr << "mapped data: " << *mapped << std::endl;
                mapped->copyAttributes(data);
#ifdef VERTTOCELL
                mapped->setMapping(DataBase::Element);
#else
                mapped->setMapping(DataBase::Vertex);
#endif
                mapped->setGrid(grid);
                updateMeta(mapped);
                task->addObject(data_out, mapped);
                return true;
            } else {
                sendError("could not handle mapped data");
                return true;
            }
        }
    }

    return true;
}
