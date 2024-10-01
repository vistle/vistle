#include <vistle/alg/objalg.h>
#include <vistle/core/unstr.h>

#include "CreateCuboids.h"

MODULE_MAIN(CreateCuboids)

using namespace vistle;

CreateCuboids::CreateCuboids(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("grid_in", "grid containing cuboid definitions (positions and edge lengths)");
    createOutputPort("grid_out", "cuboids");
}

CreateCuboids::~CreateCuboids()
{}

UnstructuredGrid::ptr createCuboidGrid(UnstructuredGrid::const_ptr midpoints, Vec<Scalar, 3>::const_ptr edgeLengths)
{
    // clang-format off
    /*
        The signs stored in this array are used to calculate the eight corner points
        of the cuboid according to vistle's hexahedron definition:
                    
                   7 -------- 6
                  /|         /|
                 / |        / |
                4 -------- 5  |        
                |  3-------|--2
                | /        | /
                |/         |/
                0----------1
        
        If mP = (mX | mY | mZ) is the midpoint of the cuboid and lX, lY, lZ are 
        the edge length along the x-, y- and z-axis, the corners are calculated 
        as follows:
        corner 0 = ( mX + lX | mY - lY | mZ - lZ), 
        corner 1 = ( mX + lX | mY + lY | mZ - lZ), ...
    */
    
    std::array<int, 24> signs = {  1, -1, -1, // corner 0
                                   1,  1, -1, // corner 1
                                  -1,  1, -1, // ...
                                  -1, -1, -1, 
                                   1, -1,  1, 
                                   1,  1,  1, 
                                  -1,  1,  1, 
                                  -1, -1,  1  
                                };
    // clang-format on

    auto numMidpoints = midpoints->getNumCoords();
    auto cornersPerCuboid = 8;

    UnstructuredGrid::ptr cuboids(
        new UnstructuredGrid(numMidpoints, cornersPerCuboid * numMidpoints, cornersPerCuboid * numMidpoints));

    for (Index i = 0; i < midpoints->getNumCoords(); i++) {
        auto x = midpoints->x()[i];
        auto y = midpoints->y()[i];
        auto z = midpoints->z()[i];

        auto lengthX = 0.5 * edgeLengths->x()[i];
        auto lengthY = 0.5 * edgeLengths->y()[i];
        auto lengthZ = 0.5 * edgeLengths->z()[i];

        for (auto j = 0; j < cornersPerCuboid; j++) {
            cuboids->x()[cornersPerCuboid * i + j] = x + lengthX * signs[3 * j];
            cuboids->y()[cornersPerCuboid * i + j] = y + lengthY * signs[3 * j + 1];
            cuboids->z()[cornersPerCuboid * i + j] = z + lengthZ * signs[3 * j + 2];

            cuboids->cl()[cornersPerCuboid * i + j] = cornersPerCuboid * i + j;
        }

        cuboids->tl()[i] = UnstructuredGrid::HEXAHEDRON;
        cuboids->el()[i] = cornersPerCuboid * i;
    }
    cuboids->el()[numMidpoints] = cornersPerCuboid * numMidpoints;

    return cuboids;
}

bool CreateCuboids::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    auto definition = task->expect<Vec<Scalar, 3>>("grid_in");
    if (!definition) {
        sendError("Mapped data must be three-dimensional!");
        return true;
    }

    auto container = splitContainerObject(definition);
    auto geo = container.geometry;
    if (!UnstructuredGrid::as(geo)) {
        sendError("The input grid must be unstructured!");
        return true;
    }
    auto centers = UnstructuredGrid::as(geo);
    auto lengths = Vec<Scalar, 3>::as(container.mapped);

    auto cuboids = createCuboidGrid(centers, lengths);

    cuboids->copyAttributes(geo);
    updateMeta(cuboids);

    task->addObject("grid_out", cuboids);

    return true;
}
