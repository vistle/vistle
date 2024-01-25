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
void AddLine(Lines::ptr lines, std::array<Scalar, 3> startPoint, std::array<Scalar, 3> endPoint)
{
    auto &x = lines->x();
    auto &y = lines->y();
    auto &z = lines->z();

    auto &cl = lines->cl();
    auto &el = lines->el();

    cl.push_back(x.size());

    x.push_back(startPoint[0]);
    y.push_back(startPoint[1]);
    z.push_back(startPoint[2]);

    cl.push_back(x.size());

    x.push_back(endPoint[0]);
    y.push_back(endPoint[1]);
    z.push_back(endPoint[2]);

    el.push_back(x.size());
}


UniformGrid::ptr CreateCellListGrid(Spheres::const_ptr spheres, Scalar searchRadius)
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

    return grid;
}

bool DetectCollision(std::array<Scalar, 3> start, std::array<Scalar, 3> end, Scalar threshold)
{
    auto distance = pow((start[0] - end[0]), 2) + pow((start[1] - end[1]), 2) + pow((start[2] - end[2]), 2);
    // spheres overlap if euclidean distance between centers is <= sum of radii
    return distance <= pow(threshold, 2);
}


/*
    Implementing the Cell Lists Algorithm to solve the Fixed-Radius Nearest Neighbors Problem , see e.g., 
    https://jaantollander.com/post/searching-for-fixed-radius-near-neighbors-with-cell-lists-algorithm-in-julia-language/
*/
Lines::ptr CellListAlgorithm(Spheres::const_ptr spheres, Scalar searchRadius)
{
    UniformGrid::ptr grid = CreateCellListGrid(spheres, searchRadius);

    // create cell list
    std::map<Index, std::vector<Index>> cellList;

    auto x = spheres->x();
    auto y = spheres->y();
    auto z = spheres->z();

    // cell list stores which sphere lies in which cell
    for (Index i = 0; i < spheres->getNumCoords(); i++) {
        auto containingCell = grid->findCell({x[i], y[i], z[i]});
        assert(containingCell != InvalidIndex);

        if (auto cell = cellList.find(containingCell); cell != cellList.end()) {
            (cell->second).push_back(i);
        } else {
            cellList[containingCell] = {i};
        }
    }

    auto radii = spheres->r();

    Lines::ptr lines(new Lines(0, 0, 0));
    (lines->el()).push_back(0);

    // draw lines between colliding spheres
    for (const auto &[cell, sphereList]: cellList) {
        auto neighbors = grid->getNeighborElements(cell);
        for (const auto sId: sphereList) {
            std::array<Scalar, 3> point1 = {x[sId], y[sId], z[sId]};
            // check for collisions with other spheres in current cell
            for (const auto sId2: sphereList) {
                if (sId < sId2) {
                    std::array<Scalar, 3> point2 = {x[sId2], y[sId2], z[sId2]};
                    if (DetectCollision(point1, point2, radii[sId] + radii[sId2])) {
                        AddLine(lines, point1, point2);
                    }
                }
            }
            // check for collisions in neighbor cells
            for (const auto neighborId: neighbors) {
                if (auto neighbor = cellList.find(neighborId); (neighbor != cellList.end() && cell < neighborId)) {
                    for (const auto nId: neighbor->second) {
                        std::array<Scalar, 3> point2 = {x[nId], y[nId], z[nId]};
                        if (DetectCollision(point1, point2, radii[sId] + radii[nId])) {
                            AddLine(lines, point1, point2);
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

    m_radiusCoefficient = addFloatParameter(
        "multiply_search_radius_by",
        "the search radius for the cell lists algorithm will be multiplied by this coefficient", 2.1);

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
    Lines::ptr lines = CellListAlgorithm(spheres, m_radiusCoefficient->getValue() * maxRadius);

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
