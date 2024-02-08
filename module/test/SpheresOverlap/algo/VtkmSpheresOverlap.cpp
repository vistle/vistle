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

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/worklet/WorkletMapField.h>

#include "PointLocatorCellLists.h"
#include "VtkmSpheresOverlap.h"

using namespace vistle;

/*
    TODO: 1-to-X mapping: 1 point can have 0..X overlaps (that is a (potentially empty) vector of Vec2i)
          --> how to do that in vtk-m?
          --> how to create a Lines object? 
                - as AtomicArray (like  vtkm::filter::density_estimate::ParticleDensityNearestGridPoint)
                - with ScatterCount (see User Guide, Chapter 32: generating cell sets)
*/
struct CreateOverlapLines: public vtkm::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn coords, ExecObject locator, FieldOut overlapIds, FieldOut thickness);
    using ExecutionSignature = void(InputIndex, _1, _2, _3, _4);

    template<typename Point, typename PointLocator, typename T>
    VTKM_EXEC void operator()(const vtkm::Id id, const Point &point, const PointLocator &locator,
                              vtkm::Vec2i &overlapIds, T thickness) const
    {
        vtkm::Id resultId;
        vtkm::FloatDefault resultFloat;

        locator.DetectOverlap(id, point, resultId, resultFloat);

        // TODO: it's not always 1 overlap per point...
        overlapIds[0] = 0;
        overlapIds[1] = 0;

        thickness = 0;
    }
};

// This should return Lines dataset with certain thickness
VTKM_CONT vtkm::cont::DataSet VtkmSpheresOverlap::DoExecute(const vtkm::cont::DataSet &inputSpheres)
{
    auto coords = inputSpheres.GetCoordinateSystem();
    auto radii = inputSpheres.GetPointField("radius");

    // create search grid
    PointLocatorCellLists pointLocator;
    pointLocator.SetCoordinates(inputSpheres.GetCoordinateSystem());

    // make search radius at least twice as large as maximum sphere radius
    pointLocator.SetSearchRadius(2.1 * radii.GetRange().ReadPortal().Get(0).Max);

    pointLocator.SetRadii(radii.GetData());

    // build search grid in parallel
    pointLocator.Update();

    // indices of the two overlapping spheres and thickness
    vtkm::cont::ArrayHandle<vtkm::Vec2i> overlapLines;
    vtkm::cont::ArrayHandle<Scalar> lineThicknesses;
    this->Invoke(CreateOverlapLines{}, pointLocator.GetCoordinates(), &pointLocator, overlapLines, lineThicknesses);
}
