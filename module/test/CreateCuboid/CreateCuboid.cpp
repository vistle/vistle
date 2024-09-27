#include "CreateCuboid.h"


MODULE_MAIN(CreateCuboid)

using namespace vistle;

CreateCuboid::CreateCuboid(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("grid_in", "middle points of the cuboids");
    createInputPort("data_in", "vectors containing the edge line size of the cuboids");

    createOutputPort("grid_out", "cuboids");
}

CreateCuboid::~CreateCuboid()
{}

bool CreateCuboid::compute()
{
    return true;
}
