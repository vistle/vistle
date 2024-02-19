#ifndef CELL_LISTS_ALGORITHM_H
#define CELL_LISTS_ALGORITHM_H

#include <vistle/core/lines.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/spheres.h>

#include "ThicknessDeterminer.h"

/*
    Creates and returns a uniform grid which encases all points in `spheres`
    and which consists of cubic cells of length `searchRadius`.
*/
vistle::UniformGrid::ptr CreateSearchGrid(vistle::Spheres::const_ptr spheres, vistle::Scalar searchRadius);

// Calculates Euclidean distance between two points
vistle::Scalar EuclideanDistance(std::array<vistle::Scalar, 3> start, std::array<vistle::Scalar, 3> end);

/*
    Saves characteristics of a line connecting two spheres.
*/
struct OverlapLineInfo {
    // indices of the two overlapping spheres
    vistle::Index id1, id2;
    vistle::Scalar thickness;

    OverlapLineInfo(vistle::Index sphereId1, vistle::Index sphereId2, vistle::Scalar distance, vistle::Scalar radius1,
                    vistle::Scalar radius2, ThicknessDeterminer determiner);
};

/*
    Uses the Cell Lists Algorithm to efficiently detect which spheres are overlapping, and creates
    lines between these spheres. Also returns the line thicknesses determined by the method defined
    by `determiner` as scalar data field.

    This algorithm solves the Fixed-Radius Nearest Neighbors Problem , see 
    https://jaantollander.com/post/searching-for-fixed-radius-near-neighbors-with-cell-lists-algorithm-in-julia-language/
*/
std::vector<OverlapLineInfo> CellListsAlgorithm(vistle::Spheres::const_ptr spheres, vistle::Scalar searchRadius,
                                                ThicknessDeterminer determiner);

/*
    Creates a line for each overlap in `overlaps` based on the coordinates provided by `spheres`.
    Returns the Lines object and a scalar data field containing the thicknesses of the line. 

    Note: Assumes that `spheres` was used to create `overlap`.
*/
std::pair<vistle::Lines::ptr, vistle::Vec<vistle::Scalar, 1>::ptr>
CreateConnectionLines(std::vector<OverlapLineInfo> overlaps, vistle::Spheres::const_ptr spheres);

#endif // CELL_LISTS_ALGORITHM_H
