#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/points.h>

#include "ToPoints.h"

MODULE_MAIN(ToPoints)

using namespace vistle;

ToPoints::ToPoints(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("grid_in");
    createOutputPort("grid_out");
}

ToPoints::~ToPoints()
{}

bool ToPoints::compute()
{
    auto d = accept<DataBase>("grid_in");
    Coords::const_ptr grid;
    if (d) {
        grid = Coords::as(d->grid());
    }
    if (!grid) {
        grid = Coords::as(d);
    }
    if (!grid) {
        sendError("no grid");
        return true;
    }

    Points::ptr points = Points::clone<Vec<Scalar, 3>>(grid);
    addObject("grid_out", points);

    return true;
}
