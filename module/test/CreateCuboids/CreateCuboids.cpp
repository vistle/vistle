#include <vistle/alg/objalg.h>
#include <vistle/core/unstr.h>

#include "CreateCuboids.h"

MODULE_MAIN(CreateCuboids)

using namespace vistle;

CreateCuboids::CreateCuboids(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("grid_in", "unstructured grid containing cuboid definitions");
    createOutputPort("grid_out", "grid containing cuboids");
}

CreateCuboids::~CreateCuboids()
{}

// returns an unstructured hexahedral grid containing the cuboids defined by `cuboidCenters` and `edgeLengths`
UnstructuredGrid::ptr createCuboidGrid(UnstructuredGrid::const_ptr cuboidCenters, Vec<Scalar, 3>::const_ptr edgeLengths)
{
    // clang-format off
    /*
        This function creates cuboids from the given centers and edge lengths following vistle's
        hexahedron definition (see lib/vistle/core/unstr.h):
                    
                   7 -------- 6
                  /|         /|
                 / |        / |
                4 -------- 5  |        
                |  3-------|--2
                | /        | /
                |/         |/
                0----------1
        
        If cP = (cX | cY | cZ) is the center of the cuboid and lX, lY, lZ are the edge length along 
        the x-, y- and z-axis, the corners are calculated as follows:
        corner 0 = ( cX + lX | cY - lY | cZ - lZ), 
        corner 1 = ( cX + lX | cY + lY | cZ - lZ), ...

        The following is the complete list of signs in front of the edge lengths for each corner:
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

    auto numCenters = cuboidCenters->getNumCoords();
    auto cornersPerCuboid = 8;

    UnstructuredGrid::ptr cuboids(
        new UnstructuredGrid(numCenters, cornersPerCuboid * numCenters, cornersPerCuboid * numCenters));

    for (Index i = 0; i < cuboidCenters->getNumCoords(); i++) {
        auto x = cuboidCenters->x()[i];
        auto y = cuboidCenters->y()[i];
        auto z = cuboidCenters->z()[i];

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
    cuboids->el()[numCenters] = cornersPerCuboid * numCenters;

    return cuboids;
}

bool CreateCuboids::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    auto definition = task->expect<Vec<Scalar, 3>>("grid_in");
    if (!definition) {
        sendError("The mapped data, i.e., the edge lengths (in x, y, z) for each cuboid, must be three-dimensional!");
        return true;
    }

    auto container = splitContainerObject(definition);
    auto geo = container.geometry;
    if (!UnstructuredGrid::as(geo)) {
        sendError("This module only supports unstructured grids!");
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
