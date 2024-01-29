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

// TODO: - instead of checking sId < sId2, adjust for range accordingly!
//       - leaving out sqrt might lead to overflow!
//       - also store distance between spheres
//       - map line width to data

/*
    Creates and returns a uniform grid which encases all points in `spheres`
    and which consists of cubic cells of length `searchRadius`.
*/
UniformGrid::ptr CreateSearchGrid(Spheres::const_ptr spheres, Scalar searchRadius)
{
    // calculate grid size using bounds of spheres
    auto bounds = spheres->getBounds();
    auto minBounds = bounds.first;
    auto maxBounds = bounds.second;

    Index dim = maxBounds.size();
    // we expect getBounds() to return a pair of Vector3s
    assert(dim == 3);

    Index gridSize[dim];

    // divide bounding box into cubes with side length = searchRadius
    for (Index i = 0; i < dim; i++) {
        gridSize[i] = ceil(maxBounds[i] / searchRadius);

        // if gridSize = 1, vistle can't create cells (since #cells = gridSize - 1),
        // To make algorithm work anyways, add second cell that does not contain any spheres
        gridSize[i] = gridSize[i] == 1 ? 2 : gridSize[i];
    }

    UniformGrid::ptr grid(new UniformGrid(gridSize[0], gridSize[1], gridSize[2]));

    // calculate grid starting and end point
    for (Index i = 0; i < dim; i++) {
        grid->min()[i] = minBounds[i];
        grid->max()[i] = minBounds[i] + gridSize[i] * searchRadius;
    }

    // apply change in grid's start and end point
    grid->refreshImpl();

    return grid;
}

// calculate Euclidean distance between two points and check if it is less or equal to `threshold`
bool DetectOverlap(std::array<Scalar, 3> start, std::array<Scalar, 3> end, Scalar threshold)
{
    // avoid calculating square root
    auto distance = pow((start[0] - end[0]), 2) + pow((start[1] - end[1]), 2) + pow((start[2] - end[2]), 2);
    return distance <= pow(threshold, 2);
}

/*
    Uses the Cell Lists Algorithm to efficiently detect which spheres are overlapping, and creates
    lines between these spheres.
    
    This algorithm solves the Fixed-Radius Nearest Neighbors Problem , see 
    https://jaantollander.com/post/searching-for-fixed-radius-near-neighbors-with-cell-lists-algorithm-in-julia-language/
*/
Lines::ptr CellListsAlgorithm(Spheres::const_ptr spheres, Scalar searchRadius)
{
    UniformGrid::ptr grid = CreateSearchGrid(spheres, searchRadius);

    // keep track of which search grid cell contains which spheres
    std::map<Index, std::vector<Index>> cellList;

    auto x = spheres->x();
    auto y = spheres->y();
    auto z = spheres->z();

    for (Index i = 0; i < spheres->getNumCoords(); i++) {
        auto containingCell = grid->findCell({x[i], y[i], z[i]});
        assert(containingCell != InvalidIndex);

        if (auto cell = cellList.find(containingCell); cell != cellList.end()) {
            (cell->second).push_back(i);
        } else {
            cellList[containingCell] = {i};
        }
    }

    // detect overlapping spheres and create line between them
    auto radii = spheres->r();

    Lines::ptr lines(new Lines(0, 0, 0));
    (lines->el()).push_back(0);

    for (const auto &[cell, sphereList]: cellList) {
        auto neighbors = grid->getNeighborElements(cell);
        for (const auto sId: sphereList) {
            std::array<Scalar, 3> point1 = {x[sId], y[sId], z[sId]};
            // check for collisions with other spheres in current cell
            for (const auto sId2: sphereList) {
                // make sure the same pair of spheres is only checked once
                if (sId < sId2) {
                    std::array<Scalar, 3> point2 = {x[sId2], y[sId2], z[sId2]};
                    // spheres overlap if distance between their centers is <= sum of radii
                    if (DetectOverlap(point1, point2, radii[sId] + radii[sId2])) {
                        lines->AddLine(point1, point2);
                    }
                }
            }
            // check for collisions in neighbor cells
            for (const auto neighborId: neighbors) {
                if (auto neighbor = cellList.find(neighborId); (neighbor != cellList.end() && cell < neighborId)) {
                    for (const auto nId: neighbor->second) {
                        std::array<Scalar, 3> point2 = {x[nId], y[nId], z[nId]};
                        if (DetectOverlap(point1, point2, radii[sId] + radii[nId])) {
                            lines->AddLine(point1, point2);
                        }
                    }
                }
            }
        }
    }

    return lines;
}

SpheresOverlap::SpheresOverlap(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    m_spheresIn = createInputPort("spheres_in", "spheres or data mapped to spheres");
    m_linesOut = createOutputPort("lines_out", "lines between all overlapping spheres");

    m_radiusCoefficient = addFloatParameter("multiply_search_radius_by",
                                            "increase search radius for the Cell Lists algorithm by this factor", 1);

    setReducePolicy(message::ReducePolicy::OverAll);
}

SpheresOverlap::~SpheresOverlap()
{}

bool SpheresOverlap::compute(const std::shared_ptr<BlockTask> &task) const
{
    auto dataIn = task->expect<Object>("spheres_in");
    auto container = splitContainerObject(dataIn);

    auto geo = container.geometry;
    auto mappedData = container.mapped;

    if (!Spheres::as(geo)) {
        sendError("input port expects spheres");
        return true;
    }

    auto spheres = Spheres::as(geo);

    auto radii = spheres->r();
    auto maxRadius = std::numeric_limits<std::remove_reference<decltype(radii[0])>::type>::min();
    for (Index i = 0; i < radii.size(); i++) {
        if (radii[i] > maxRadius)
            maxRadius = radii[i];
    }

    // We want to ensure that no pair of overlapping spheres is missed. As two spheres overlap when
    // the Euclidean distance between them is less than the sum of their radii, the minimum search
    // radius is two times the maximum sphere radius.
    Lines::ptr lines = CellListsAlgorithm(spheres, 2.1 * m_radiusCoefficient->getValue() * maxRadius);

    if (lines->getNumCoords()) {
        if (mappedData) {
            lines->copyAttributes(mappedData);
        } else {
            lines->copyAttributes(geo);
        }

        updateMeta(lines);
        task->addObject(m_linesOut, lines);
    }
    return true;
}
