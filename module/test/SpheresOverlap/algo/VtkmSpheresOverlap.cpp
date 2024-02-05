/*
    1.) convert vistle to vtk-m : Spheres to CellSet (of any type) where coordinates
                                  are sphere coordinates and data field is radius
    2.) create search grid: uniform grid, cell size = search radius, bounds =
                            sphere bounds (probably best to make it search grid from 1.))
    3.) create cell list: assign each sphere to cell containing it 
    4.) use Point Neighborhood Worklet for checking overlap between 
        spheres in this cell & neighbor cells
    5.) convert result (lines) back to vistle structure

    For 1.) and 2.): ParticleDensityBase.h seems to be doing that

*/

#include <vistle/core/spheres.h>
#include <vistle/core/uniformgrid.h>

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/worklet/WorkletCellNeighborhood.h>

#include "VtkmSpheresOverlap.h"

using namespace vistle;

class DetectOverlap: public vtkm::worklet::WorkletCellNeighborhood {
public:
    using ControlSignature = void(CellSetIn);

    using ExecutionSignature = void(_1);

    template<typename Boundary>
    VTKM_EXEC void operator()(Boundary boundary) const
    {
        //auto thisPosition = neighborPosition.Get(0, 0, 0);
        //auto thisRadius = neighborRadius.Get(0, 0, 0);

        auto minIndices = boundary.MinNeighborIndices(1);
        auto maxIndices = boundary.MaxNeighborIndices(1);

        // go through all neighbor cells
        for (int k = minIndices[2]; k <= maxIndices[2]; k++) {
            for (int j = minIndices[1]; j <= maxIndices[1]; j++) {
                for (int i = minIndices[0]; i <= maxIndices[0]; i++) {
                    // find all spheres in neighbor cell and compare them with current spheres

                    // check if they overlap
                    //vtkm::Magnitude()
                }
            }
        }
    }
};

vtkm::cont::DataSet CreateSearchGrid(Spheres::const_ptr spheres, Scalar searchRadius)
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

    vtkm::cont::DataSetBuilderUniform datasetBuilder;
    return datasetBuilder.Create(vtkm::Id3(gridSize[0], gridSize[1], gridSize[2]),
                                 vtkm::Vec3f(minBounds[0], minBounds[1], minBounds[2]),
                                 vtkm::Vec3f(searchRadius, searchRadius, searchRadius));
}
