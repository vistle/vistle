/*
    TODO: add Spheres to vistle/vtkm/convert
*/

#include <vistle/core/spheres.h>
#include <vistle/core/uniformgrid.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/PointLocatorSparseGrid.h>
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
    using ControlSignature = void(FieldIn coords, ExecObject locator, FieldOut overlaps);
    using ExecutionSignature = void(InputIndex, _1, _2, _3);

    template<typename Point, typename PointLocatorExecObject>
    VTKM_EXEC void operator()(const vtkm::Id id, const Point &point, const PointLocatorExecObject &locator,
                              vtkm::cont::ArrayHandle<vtkm::Vec2i>& overlaps) const
    {
        //locator.CountOverlaps(id, point, overlaps);
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
    vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec2i>> overlaps;

    // this is how to add an element to overlaps:
    /*
    vtkm::Vec2i test = {0, 0};
    vtkm::cont::ArrayHandle<vtkm::Vec2i> testAh;
    testAh.Allocate(2);
    testAh.WritePortal().Set(0, test);
    testAh.WritePortal().Set(1, test);
    
    vtkm::cont::ArrayHandle<vtkm::Vec2i> testAh2;
    testAh2.Allocate(3);
    testAh2.WritePortal().Set(0, test);
    testAh2.WritePortal().Set(1, test);
    testAh2.WritePortal().Set(2, test);

    overlaps.Allocate(2);
    overlaps.WritePortal().Set(0, testAh);
    overlaps.WritePortal().Set(1, testAh2);
    */

    vtkm::cont::PointLocatorSparseGrid loc;
    loc.SetCoordinates(inputSpheres.GetCoordinateSystem());
    this->Invoke(CreateOverlapLines{}, loc.GetCoordinates(), &loc, overlaps);
}
