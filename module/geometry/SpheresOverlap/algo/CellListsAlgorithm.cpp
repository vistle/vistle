#include "CellListsAlgorithm.h"

using namespace vistle;

UniformGrid::ptr CreateSearchGrid(Points::const_ptr spheres, Scalar searchRadius)
{
    // calculate grid size using bounds of spheres
    auto bounds = spheres->getBounds();
    auto minBounds = bounds.first;
    auto maxBounds = bounds.second;

    Index dim = maxBounds.size();
    // we expect getBounds() to return a pair of Vector3s
    assert(dim == 3);

    // FIXME: Check if this is supported by all compilers once eigen v3.5 is released
    //        (currently it's not supported by MSVC-) 
    //Index gridSize[dim];
    Index gridSize[3];

    // divide bounding box into cubes with side length = searchRadius
    for (Index i = 0; i < dim; i++) {
        gridSize[i] = ceil((maxBounds[i] - minBounds[i]) / searchRadius);

        // if gridSize = 1, vistle can't create cells (since #cells = gridSize - 1),
        // To make algorithm work anyways, add second cell that does not contain any spheres
        gridSize[i] = gridSize[i] == 1 ? 2 : gridSize[i];
    }

    UniformGrid::ptr grid(new UniformGrid(gridSize[0] + 1, gridSize[1] + 1, gridSize[2] + 1));

    // calculate grid starting and end point
    for (Index i = 0; i < dim; i++) {
        grid->min()[i] = minBounds[i];
        grid->max()[i] = minBounds[i] + gridSize[i] * searchRadius;
    }

    // apply change in grid's start and end point
    grid->refreshImpl();

    return grid;
}

Scalar EuclideanDistance(std::array<Scalar, 3> start, std::array<Scalar, 3> end)
{
    return sqrt(pow((start[0] - end[0]), 2) + pow((start[1] - end[1]), 2) + pow((start[2] - end[2]), 2));
}

OverlapLineInfo::OverlapLineInfo(Index sphereId1, Index sphereId2, Scalar distance, Scalar radius1, Scalar radius2,
                                 ThicknessDeterminer determiner)
: id1(sphereId1), id2(sphereId2)
{
    thickness = CalculateThickness(determiner, distance, radius1, radius2);
}

// returns a vector containing the current cell's id as well as the ids of all
// neighbor cells (including diagonal neighbors)
std::vector<Index> getCellsToCheck(UniformGrid::const_ptr grid, Index elem)
{
    std::vector<Index> elems;
    if (elem == InvalidIndex)
        return elems;

    const Index dims[3] = {grid->getNumDivisions(0), grid->getNumDivisions(1), grid->getNumDivisions(2)};
    const auto coords = grid->cellCoordinates(elem, dims);

    auto c = coords;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            for (int k = -1; k <= 1; ++k) {
                c = {coords[0] + i, coords[1] + j, coords[2] + k};

                if ((c[0] >= 0 && c[0] < dims[0] - 1) && (c[1] >= 0 && c[1] < dims[1] - 1) &&
                    (c[2] >= 0 && c[2] < dims[2] - 1))
                    elems.push_back(grid->cellIndex(c[0], c[1], c[2], dims));
            }
        }
    }
    return elems;
}

std::vector<OverlapLineInfo> CellListsAlgorithm(Points::const_ptr spheres, Scalar searchRadius,
                                                ThicknessDeterminer determiner)
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

    // detect overlapping spheres and save overlap info
    auto radii = spheres->radius()->x();
    std::vector<OverlapLineInfo> overlaps;

    for (const auto &[cell, sphereList]: cellList) {
        for (const auto sId: sphereList) {
            std::array<Scalar, 3> point1 = {x[sId], y[sId], z[sId]};
            auto radius1 = radii[sId];
            // check for collisions in current and neighbor cells
            for (const auto cellToCheck: getCellsToCheck(grid, cell)) {
                if (auto idsToCheck = cellList.find(cellToCheck);
                    (idsToCheck != cellList.end() && cell <= cellToCheck)) {
                    for (const auto toCheck: idsToCheck->second) {
                        // if two overlapping spheres lie in the same cell, we can avoid checking the same pair twice
                        // by only checking the point with the "smaller" id for overlap (note that the points are not sorted)
                        if ((cell != cellToCheck && sId != toCheck) || (cell == cellToCheck && sId < toCheck)) {
                            std::array<Scalar, 3> point2 = {x[toCheck], y[toCheck], z[toCheck]};
                            Scalar radius2 = radii[toCheck];
                            if (auto distance = EuclideanDistance(point1, point2); distance <= radius1 + radius2)
                                overlaps.push_back({sId, toCheck, distance, radius1, radius2, determiner});
                        }
                    }
                }
            }
        }
    }

    return overlaps;
}

std::pair<Lines::ptr, Vec<Scalar, 1>::ptr> CreateConnectionLines(std::vector<OverlapLineInfo> overlaps,
                                                                 Points::const_ptr spheres)
{
    auto nrOverlaps = overlaps.size();

    Lines::ptr lines(new Lines(nrOverlaps, 2 * nrOverlaps, 2 * nrOverlaps));
    Vec<Scalar, 1>::ptr thicknesses(new Vec<Scalar, 1>(nrOverlaps));

    auto xSpheres = spheres->x();
    auto ySpheres = spheres->y();
    auto zSpheres = spheres->z();

    auto xLines = lines->x().data();
    auto yLines = lines->y().data();
    auto zLines = lines->z().data();

    lines->el().data()[0] = 0;
    for (size_t i = 0; i < nrOverlaps; i++) {
        xLines[2 * i] = xSpheres[overlaps[i].id1];
        yLines[2 * i] = ySpheres[overlaps[i].id1];
        zLines[2 * i] = zSpheres[overlaps[i].id1];

        xLines[2 * i + 1] = xSpheres[overlaps[i].id2];
        yLines[2 * i + 1] = ySpheres[overlaps[i].id2];
        zLines[2 * i + 1] = zSpheres[overlaps[i].id2];

        (lines->cl().data())[2 * i] = 2 * i;
        (lines->cl().data())[2 * i + 1] = 2 * i + 1;

        (lines->el().data())[i + 1] = 2 * (i + 1);

        (thicknesses->x().data())[i] = overlaps[i].thickness;
    }

    return {lines, thicknesses};
}
