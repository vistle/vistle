#include <vistle/alg/objalg.h>
#include <vistle/core/unstr.h>

#include "CreateCuboid.h"

MODULE_MAIN(CreateCuboid)

using namespace vistle;

CreateCuboid::CreateCuboid(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("grid_in", "grid containing cuboid definitions (positions and edge lengths)");
    createOutputPort("grid_out", "cuboids");
}

CreateCuboid::~CreateCuboid()
{}

bool CreateCuboid::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    UnstructuredGrid::ptr cuboids;

    auto cuboidDef = task->expect<Vec<Scalar, 3>>("grid_in");
    if (!cuboidDef) {
        sendError("Mapped data must be three-dimensional!");
        return true;
    }

    auto defContainer = splitContainerObject(cuboidDef);
    auto points = defContainer.geometry;
    auto lengths = defContainer.mapped;

    return true;
}
