#include "CellListsAlgorithm.h"

using namespace vistle;

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
        gridSize[i] = ceil((maxBounds[i] - minBounds[i]) / searchRadius);

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

Scalar EuclideanDistance(std::array<Scalar, 3> start, std::array<Scalar, 3> end)
{
    return sqrt(pow((start[0] - end[0]), 2) + pow((start[1] - end[1]), 2) + pow((start[2] - end[2]), 2));
}

OverlapLineInfo::OverlapLineInfo(Index sphereId1, Index sphereId2, Scalar distance, Scalar radius1, Scalar radius2,
                                 ThicknessDeterminer determiner)
: id1(sphereId1), id2(sphereId2)
{
    switch (determiner) {
    case Overlap:
        thickness = abs(radius1 + radius2 - distance);
        break;
    case OverlapRatio:
        thickness = abs(radius1 + radius2 - distance) / distance;
        break;
    case Distance:
        thickness = distance;
        break;
    default:
        throw std::invalid_argument("Unknown thickness determiner!");
    }
}

std::vector<OverlapLineInfo> CellListsAlgorithm(Spheres::const_ptr spheres, Scalar searchRadius,
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
    auto radii = spheres->r();
    std::vector<OverlapLineInfo> overlaps;

    for (const auto &[cell, sphereList]: cellList) {
        for (const auto sId: sphereList) {
            std::array<Scalar, 3> point1 = {x[sId], y[sId], z[sId]};
            auto radius1 = radii[sId];

            // check for collisions with other spheres in current cell
            for (const auto sId2: sphereList) {
                // make sure the same pair of spheres is only checked once
                if (sId < sId2) {
                    std::array<Scalar, 3> point2 = {x[sId2], y[sId2], z[sId2]};
                    Scalar radius2 = radii[sId2];
                    // spheres overlap if distance between their centers is <= sum of radii
                    if (auto distance = EuclideanDistance(point1, point2); distance <= radius1 + radius2)
                        overlaps.push_back({sId, sId2, distance, radius1, radius2, determiner});
                }
            }
            // check for collisions in neighbor cells
            for (const auto neighborId: grid->getNeighborElements(cell)) {
                if (auto neighbor = cellList.find(neighborId); (neighbor != cellList.end() && cell < neighborId)) {
                    for (const auto nId: neighbor->second) {
                        if (sId < nId) {
                            std::array<Scalar, 3> point2 = {x[nId], y[nId], z[nId]};
                            Scalar radius2 = radii[nId];
                            if (auto distance = EuclideanDistance(point1, point2); distance <= radius1 + radius2)
                                overlaps.push_back({sId, nId, distance, radius1, radius2, determiner});
                        }
                    }
                }
            }
        }
    }

    return overlaps;
}

std::pair<Lines::ptr, Vec<Scalar, 1>::ptr> CreateConnectionLines(std::vector<OverlapLineInfo> overlaps,
                                                                 Spheres::const_ptr spheres)
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