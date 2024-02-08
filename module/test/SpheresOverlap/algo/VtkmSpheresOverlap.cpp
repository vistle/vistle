/*
    1.) create uniform search grid with cont::PointLocatorSearchGrid which gives us
        --> PointIds: a list which stores all point ids sorted (by key) by the ids of 
            the cells that contain them
        --> LowerBounds: entry i stores the first index of the points in PointsId which
                         lie in cell i
        --> UpperBounds: entry i stores the last index of the points in PointsId which
                         lie in cell i
            
    2.) write function which gets the indices of all neighboring cells 
        (maybe look at FindInBox?)

    3.) write function for finding overlap similar to exec::PointLocatorSearchGrid::FindCell 

    4.) think about data structure for saving Lines object (has to be convertible to vistle
        to work with OpenCOVER and rest of modules!)
*/

#include <vistle/core/spheres.h>
#include <vistle/core/uniformgrid.h>

#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/DataSetBuilderUniform.h>

#include <vtkm/exec/CellEdge.h>

#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/ScatterUniform.h>
#include <vtkm/worklet/WorkletCellNeighborhood.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include "PointLocatorCellLists.h"
#include "VtkmSpheresOverlap.h"

using namespace vistle;

// return the two indices needed to create a line

// extracts all edges from a cell set
class EdgeIndicesWorklet: public vtkm::worklet::WorkletVisitCellsWithPoints {
public:
    using ControlSignature = void(CellSetIn cellSet, FieldOut connectivity);
    using ExecutionSignature = void(CellShape, PointIndices, _2, VisitIndex);

    using ScatterType = vtkm::worklet::ScatterCounting;

    template<typename CellShapeTag, typename PointIndexVecType>
    VTKM_EXEC void operator()(CellShapeTag cellShape, const PointIndexVecType &globalPointIndicesForCell,
                              vtkm::Id2 &connectivity, vtkm::IdComponent edgeIndex) const
    {
        vtkm::IdComponent numPointsInCell = globalPointIndicesForCell.GetNumberOfComponents();
        vtkm::IdComponent pointInCellIndex0;
        vtkm::exec::CellEdgeLocalIndex(numPointsInCell, 0, edgeIndex, cellShape, pointInCellIndex0);
        vtkm::IdComponent pointInCellIndex1;
        vtkm::exec::CellEdgeLocalIndex(numPointsInCell, 1, edgeIndex, cellShape, pointInCellIndex1);

        connectivity[0] = globalPointIndicesForCell[pointInCellIndex0];
        connectivity[1] = globalPointIndicesForCell[pointInCellIndex1];
    }
};

class DetectOverlap: public vtkm::worklet::WorkletCellNeighborhood {
public:
    using ControlSignature = void(CellSetIn, FieldInNeighborhood coords, FieldInNeighborhood radii, ExecObject locator,
                                  AtomicArrayInOut thicknesses);

    using ExecutionSignature = void(Boundary, _2, _3, _4, _5);

    template<typename Boundary, typename NeighborPoint, typename NeighborRadius, typename CellLocatorExecObj,
             typename AtomicArray>
    VTKM_EXEC void operator()(Boundary boundary, const NeighborPoint &neighborPoint,
                              const NeighborRadius neighborRadius, const CellLocatorExecObj &locator,
                              AtomicArray &thicknesses) const
    {
        auto currentPosition = neighborPosition.Get(0, 0, 0);
        auto currentRadius = neighborRadius.Get(0, 0, 0);

        auto minIndices = boundary.MinNeighborIndices(1);
        auto maxIndices = boundary.MaxNeighborIndices(1);

        vtkm::Id cellId{};

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


// This should return Lines dataset with certain thickness
VTKM_CONT vtkm::cont::DataSet VtkmSpheresOverlap::DoExecute(const vtkm::cont::DataSet &inputSpheres)
{
    auto coords = inputSpheres.GetCoordinateSystem();
    auto radii = inputSpheres.GetPointField("radius");

    vtkm::cont::ArrayHandle<Scalar> thickness; // output

    // determine number of lines and connectivity array
    /*auto nrOverlaps = 0;
    vtkm::worklet::ScatterUniform<2> scatter;

    auto inputCells = inputSpheres.GetCellSet();
    vtkm::cont::ArrayHandle<vtkm::Id> linesConnectivity;

    // need to use ArrayHandleGroupVec as CellSetSingleType expects a flat array
    this->Invoke(EdgeIndicesWorklet{}, scatter, inputCells, vtkm::cont::make_ArrayHandleGroupVec<2>(linesConnectivity));

    vtkm::cont::CellSetSingleType<> overlapLines;
    overlapLines.Fill(nrOverlaps, vtkm::CELL_SHAPE_LINE, 2, linesConnectivity);
    */

    // create search grid
    PointLocatorCellLists pointLocator;
    pointLocator.SetCoordinates(inputSpheres.GetCoordinateSystem());

    // make search radius at least twice as large as maximum sphere radius
    pointLocator.SetSearchRadius(2.1 * radii.GetRange().ReadPortal().Get(0).Max);

    pointLocator.SetRadii(radii.GetData());

    // build search grid in parallel
    pointLocator.Update();
}
