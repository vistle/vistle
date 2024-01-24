#include <map>
#include <math.h>
#include <type_traits>
#include <vector>

#include <vistle/alg/objalg.h>
#include <vistle/core/lines.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/spheres.h>

#include "SpheresOverlap.h"

using namespace vistle;

MODULE_MAIN(SpheresOverlap)

// TODO: check overlap between points in one cell!

/*
    Implementing the Cell Lists Algorithm to solve the Fixed-Radius Nearest Neighbors Problem , see e.g., 
    https://jaantollander.com/post/searching-for-fixed-radius-near-neighbors-with-cell-lists-algorithm-in-julia-language/
*/
Lines::ptr CellListAlgorithm(Spheres::const_ptr spheres, Scalar searchRadius)
{
    // calculate grid size using bounds of spheres
    auto bounds = spheres->getBounds();
    auto minBounds = bounds.first;
    auto maxBounds = bounds.second;

    Index dim = maxBounds.size();
    // getBounds() always returns Vector3
    assert(dim == 3);

    Index gridSize[dim];

    // divide bounding box into cubes with side length = searchRadius
    for (Index i = 0; i < dim; i++) {
        gridSize[i] = ceil(maxBounds[i] / searchRadius);
        // if gridSize is 1, viste does not create cells (since #cells = gridSize - 1),
        // so in these cases add second cell that is empty
        gridSize[i] = gridSize[i] == 1 ? 2 : gridSize[i];
    }

    UniformGrid::ptr grid(new UniformGrid(gridSize[0], gridSize[1], gridSize[2]));

    // calculate grid starting and end point
    for (Index i = 0; i < dim; i++) {
        grid->min()[i] = minBounds[i];
        grid->max()[i] = minBounds[i] + gridSize[i] * searchRadius;
    }

    grid->refreshImpl();
    // create cell list
    std::map<Index, std::vector<Index>> cellList;

    auto xSpheres = spheres->x();
    auto ySpheres = spheres->y();
    auto zSpheres = spheres->z();

    // cell list stores which sphere lies in which cell
    for (Index i = 0; i < spheres->getNumCoords(); i++) {
        auto containingCell = grid->findCell({xSpheres[i], ySpheres[i], zSpheres[i]});
        assert(containingCell != InvalidIndex);

        if (auto cell = cellList.find(containingCell); cell != cellList.end()) {
            (cell->second).push_back(i);
        } else {
            cellList[containingCell] = {i};
        }
    }

    /* go through each cell
           go through each neighbor cell  but try not to test same pair TWICE 
           (only test LARGER indices)
               if spheres overlap:
                    create connection between them
    */
    auto radii = spheres->r();

    Lines::ptr lines(new Lines(0, 0, 0));

    auto &xLines = lines->x();
    auto &yLines = lines->y();
    auto &zLines = lines->z();

    auto &clLines = lines->cl();
    auto &elLines = lines->el();
    elLines.push_back(0);
    Index numCoordsLines = 0;


    for (const auto &[cell, sphereList]: cellList) {
        auto neighbors = grid->getNeighborElements(cell);
        for (const auto neighborId: neighbors) {
            // avoid testing the same two cells twice
            if (cell < neighborId) {
                // make sure neighbor cell contains spheres, too
                if (auto neighbor = cellList.find(neighborId); neighbor != cellList.end()) {
                    for (const auto sId: sphereList) {
                        for (const auto nId: neighbor->second) {
                            // no need to compute sqrt for euclidean distance
                            auto distance = pow((xSpheres[sId] - xSpheres[nId]), 2) +
                                            pow((ySpheres[sId] - ySpheres[nId]), 2) +
                                            pow((zSpheres[sId] - zSpheres[nId]), 2);
                            // spheres overlap if euclidean distance between centers is <= sum of radii
                            if (distance <= pow(radii[sId] + radii[nId], 2)) {
                                xLines.push_back(xSpheres[sId]);
                                yLines.push_back(ySpheres[sId]);
                                zLines.push_back(zSpheres[sId]);

                                xLines.push_back(xSpheres[nId]);
                                yLines.push_back(ySpheres[nId]);
                                zLines.push_back(zSpheres[nId]);

                                clLines.push_back(numCoordsLines++);
                                clLines.push_back(numCoordsLines++);

                                elLines.push_back(numCoordsLines);
                            }
                        }
                    }
                }
            }
        }
    }

    return lines;
}


/*
    TODO: 
    - maybe rename to SphereNetwork? -> also change port descriptions
*/
SpheresOverlap::SpheresOverlap(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    m_spheresIn = createInputPort("spheres_in", "spheres for which overlap will be calculated");
    m_linesOut = createOutputPort("lines_out", "lines between all overlapping spheres");

    setReducePolicy(message::ReducePolicy::OverAll);
}

SpheresOverlap::~SpheresOverlap()
{}

bool SpheresOverlap::compute(const std::shared_ptr<BlockTask> &task) const
{
    auto dataIn = task->expect<Object>("spheres_in");
    auto splitDataIn = splitContainerObject(dataIn);

    if (!Spheres::as(splitDataIn.geometry)) {
        sendError("input port expects spheres");
        return true;
    }

    auto spheres = Spheres::as(splitDataIn.geometry);

    auto radii = spheres->r();
    auto maxRadius = std::numeric_limits<std::remove_reference<decltype(radii[0])>::type>::min();
    for (Index i = 0; i < radii.size(); i++) {
        if (radii[i] > maxRadius)
            maxRadius = radii[i];
    }
    Lines::ptr lines = CellListAlgorithm(spheres, 3 * maxRadius);

    if (lines->getNumCoords()) {
        lines->copyAttributes(spheres);
        updateMeta(lines);
        task->addObject(m_linesOut, lines);
    }

    return true;
}
