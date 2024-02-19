#include <vistle/core/spheres.h>
#include <vistle/core/uniformgrid.h>

#include <vtkm/cont/Algorithm.h> // TODO: delete (only used for reduce in assert)
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/CellSetSingleType.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/ScatterCounting.h>

#include "worklet/PointLocatorCellLists.h"
#include "VtkmSpheresOverlap.h"

using namespace vistle;

struct CountOverlaps: public vtkm::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn coords, ExecObject locator, FieldOut overlaps);
    using ExecutionSignature = void(InputIndex, _1, _2, _3);

    template<typename Point, typename PointLocatorExecObject>
    VTKM_EXEC void operator()(const vtkm::Id id, const Point &point, const PointLocatorExecObject &locator,
                              vtkm::Id &nrOverlaps) const
    {
        locator.CountOverlaps(id, point, nrOverlaps);
    }
};

struct CreateOverlapLines: public vtkm::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn coords, ExecObject locator, FieldOut linesConnectivity,
                                  FieldOut linesThickness);
    using ExecutionSignature = void(InputIndex, _1, _2, VisitIndex, _3, _4);

    using ScatterType = vtkm::worklet::ScatterCounting;

    template<typename CountArrayType>
    VTKM_CONT static ScatterType MakeScatter(const CountArrayType &countArray)
    {
        VTKM_IS_ARRAY_HANDLE(CountArrayType);
        return ScatterType(countArray);
    }

    template<typename Point, typename PointLocatorExecObject>
    VTKM_EXEC void operator()(const vtkm::Id id, const Point &point, const PointLocatorExecObject &locator,
                              const vtkm::IdComponent visitId, vtkm::Id2 &connectivity,
                              vtkm::FloatDefault &thickness) const
    {
        locator.CreateOverlapLines(id, point, visitId, connectivity, thickness);
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
    pointLocator.SetSearchRadius(this->RadiusFactor * 2 * radii.GetRange().ReadPortal().Get(0).Max);

    pointLocator.SetRadii(radii.GetData());

    pointLocator.SetThicknessDeterminer(this->Determiner);

    // build search grid in parallel
    pointLocator.Update();

    // first count the overlaps per point
    vtkm::cont::ArrayHandle<vtkm::Id> overlapsPerPoint;

    this->Invoke(CountOverlaps{}, pointLocator.GetCoordinates(), &pointLocator, overlapsPerPoint);

    // then create lines between all overlapping spheres
    // mapping is not 1-to-1 since a sphere can overlap with arbitrarily many other spheres (including none)
    auto spheresToLinesMapping = CreateOverlapLines::MakeScatter(overlapsPerPoint);

    vtkm::cont::ArrayHandle<vtkm::Id> linesConnectivity;
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> lineThicknesses;

    this->Invoke(CreateOverlapLines{}, spheresToLinesMapping, pointLocator.GetCoordinates(), &pointLocator,
                 vtkm::cont::make_ArrayHandleGroupVec<2>(linesConnectivity), lineThicknesses);

    assert(linesConnectivity.GetNumberOfValues() == 2 * vtkm::cont::Algorithm::Reduce(overlapsPerPoint, 0));
    vtkm::cont::CellSetSingleType<> linesCellSet;
    linesCellSet.Fill(linesConnectivity.GetNumberOfValues(), vtkm::CELL_SHAPE_LINE, 2, linesConnectivity);

    vtkm::cont::DataSet overlapLines;

    // make sure vtkms understands that this dataset is empty, if no overlaps are found
    if (linesConnectivity.GetNumberOfValues() > 0)
        overlapLines.AddCoordinateSystem(coords);

    overlapLines.SetCellSet(linesCellSet);
    overlapLines.AddCellField("lineThickness", lineThicknesses);

    return overlapLines;
}
