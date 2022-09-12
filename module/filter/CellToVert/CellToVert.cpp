#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/triangles.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>
#include <vistle/alg/objalg.h>

#include "CellToVert.h"
#include "coCellToVert.h"

using namespace vistle;

using namespace vistle;

MODULE_MAIN(CellToVert)

CellToVert::CellToVert(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    for (int i = 0; i < NumPorts; ++i) {
        std::string in("data_in");
        std::string out("data_out");
        if (i > 0) {
            in += std::to_string(i);
            out += std::to_string(i);
        }
        m_data_in.push_back(createInputPort(in));
        m_data_out.push_back(createOutputPort(out));
    }
}

CellToVert::~CellToVert()
{}

bool CellToVert::compute(std::shared_ptr<BlockTask> task) const
{
    coCellToVert algo;

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
        if (mapping == DataBase::Vertex) {
            auto ndata = data->clone();
            ndata->setGrid(grid);
            ndata->setMapping(DataBase::Vertex);
            updateMeta(ndata);
            task->addObject(data_out, ndata);
        } else {
            DataBase::ptr out = algo.interpolate(grid, data);
            if (out) {
                out->copyAttributes(data);
                out->setGrid(grid);
                out->setMapping(DataBase::Vertex);
                updateMeta(out);
                task->addObject(data_out, out);
            } else {
                std::stringstream str;
                str << "could not remap " << data->getName();
                std::string s = str.str();
                sendError("%s", s.c_str());
                return true;
            }
        }
    }

    return true;
}
