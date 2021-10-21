#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/triangles.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>

#include "CellToVert.h"
#include "coCellToVert.h"

using namespace vistle;

using namespace vistle;

MODULE_MAIN(CellToVert)

CellToVert::CellToVert(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("data_in");

    createOutputPort("data_out");
}

CellToVert::~CellToVert()
{}

bool CellToVert::compute(std::shared_ptr<PortTask> task) const
{
    coCellToVert algo;

    auto data = task->expect<DataBase>("data_in");
    if (!data)
        return true;

    auto grid = data->grid();
    if (!grid) {
        sendError("grid is required on input object");
        return true;
    }

    auto mapping = data->guessMapping();
    if (mapping == DataBase::Vertex) {
        auto ndata = data->clone();
        task->addObject("data_out", ndata);
    } else if (mapping != DataBase::Element) {
        std::stringstream str;
        str << "unsupported data mapping " << data->mapping() << ", guessed=" << mapping << " on " << data->getName();
        std::string s = str.str();
        sendError("%s", s.c_str());
        return true;
    }

    DataBase::ptr out = algo.interpolate(grid, data);
    if (out) {
        out->copyAttributes(data);
        out->setGrid(grid);
        out->setMapping(DataBase::Vertex);
        task->addObject("data_out", out);
    }

    return true;
}
