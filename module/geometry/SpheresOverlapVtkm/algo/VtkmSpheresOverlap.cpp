#include <vistle/core/uniformgrid.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/CellSetSingleType.h>

#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapField.h>

#include "VtkmSpheresOverlap.h"

#ifdef NONSEPARABLE_COMPILATION
#include "ThicknessDeterminer.cpp"
#include "worklet/PointLocatorCellLists.cpp"
#include "worklet/OverlapDetector.cpp"
#endif


using namespace vistle;

struct CountOverlapsWorklet: public viskores::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn coords, ExecObject locator, FieldOut overlaps);
    using ExecutionSignature = void(InputIndex, _1, _2, _3);

    template<typename Point, typename PointLocatorExecObject>
    VISKORES_EXEC void operator()(const viskores::Id id, const Point &point, const PointLocatorExecObject &locator,
                                  viskores::Id &nrOverlaps) const
    {
        locator.CountOverlaps(id, point, nrOverlaps);
    }
};

struct CreateConnectionLinesWorklet: public viskores::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn coords, ExecObject locator, FieldOut linesConnectivity,
                                  FieldOut linesThickness);
    using ExecutionSignature = void(InputIndex, _1, _2, VisitIndex, _3, _4);

    using ScatterType = viskores::worklet::ScatterCounting;

    template<typename CountArrayType>
    VISKORES_CONT static ScatterType MakeScatter(const CountArrayType &countArray)
    {
        VISKORES_IS_ARRAY_HANDLE(CountArrayType);
        return ScatterType(countArray);
    }

    template<typename Point, typename PointLocatorExecObject>
    VISKORES_EXEC void operator()(const viskores::Id id, const Point &point, const PointLocatorExecObject &locator,
                                  const viskores::IdComponent visitId, viskores::Id2 &connectivity,
                                  viskores::FloatDefault &thickness) const
    {
        locator.CreateConnectionLines(id, point, visitId, connectivity, thickness);
    }
};

VISKORES_CONT PointLocatorCellLists VtkmSpheresOverlap::CreateSearchGrid(
    const viskores::cont::CoordinateSystem &coordinates, const viskores::cont::Field &radii)
{
    PointLocatorCellLists result;
    result.SetCoordinates(coordinates);
    result.SetRadii(radii.GetData());

    // make search radius at least twice as large as maximum sphere radius
    result.SetSearchRadius(this->RadiusFactor * 2 * radii.GetRange().ReadPortal().Get(0).Max);
    result.SetThicknessDeterminer(this->Determiner);
    result.Update();

    return result;
}

VISKORES_CONT viskores::cont::ArrayHandle<viskores::Id>
VtkmSpheresOverlap::CountOverlapsPerPoint(const PointLocatorCellLists &pointLocator)
{
    viskores::cont::ArrayHandle<viskores::Id> result;
    this->Invoke(CountOverlapsWorklet{}, pointLocator.GetCoordinates(), &pointLocator, result);
    return result;
}

VISKORES_CONT void
VtkmSpheresOverlap::CreateConnectionLines(const PointLocatorCellLists &pointLocator,
                                          const viskores::cont::ArrayHandle<viskores::Id> &overlapsPerPoint,
                                          viskores::cont::ArrayHandle<viskores::Id> &linesConnectivity,
                                          viskores::cont::ArrayHandle<viskores::FloatDefault> &linesThicknesses)
{
    // mapping is not 1-to-1 since a sphere can overlap with arbitrarily many other spheres (including none)
    auto spheresToLinesMapping = CreateConnectionLinesWorklet::MakeScatter(overlapsPerPoint);

    this->Invoke(CreateConnectionLinesWorklet{}, spheresToLinesMapping, pointLocator.GetCoordinates(), &pointLocator,
                 viskores::cont::make_ArrayHandleGroupVec<2>(linesConnectivity), linesThicknesses);
}

VISKORES_CONT viskores::cont::DataSet
CreateDataset(const viskores::cont::CoordinateSystem &coordinates,
              const viskores::cont::ArrayHandle<viskores::Id> &linesConnectivity,
              const viskores::cont::ArrayHandle<viskores::FloatDefault> &linesThicknesses)
{
    viskores::cont::CellSetSingleType<> linesCellSet;
    linesCellSet.Fill(linesConnectivity.GetNumberOfValues(), viskores::CELL_SHAPE_LINE, 2, linesConnectivity);

    viskores::cont::DataSet result;
    linesCellSet.GetNumberOfPoints();

    // make sure vtkm understands that this dataset is empty, if no overlaps are found
    if (linesConnectivity.GetNumberOfValues() > 0)
        result.AddCoordinateSystem(coordinates);

    result.SetCellSet(linesCellSet);
    result.AddCellField("lineThickness", linesThicknesses);
    return result;
}

VISKORES_CONT viskores::cont::DataSet VtkmSpheresOverlap::DoExecute(const viskores::cont::DataSet &spheres)
{
    auto sphereCenters = spheres.GetCoordinateSystem();

    auto pointLocator = CreateSearchGrid(sphereCenters, spheres.GetPointField("radius"));
    auto overlapsPerPoint = CountOverlapsPerPoint(pointLocator);

    viskores::cont::ArrayHandle<viskores::Id> linesConnectivity;
    viskores::cont::ArrayHandle<viskores::FloatDefault> linesThicknesses;
    CreateConnectionLines(pointLocator, overlapsPerPoint, linesConnectivity, linesThicknesses);

    return CreateDataset(sphereCenters, linesConnectivity, linesThicknesses);
}
