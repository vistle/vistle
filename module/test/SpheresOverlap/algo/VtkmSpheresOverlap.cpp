#include <vistle/core/uniformgrid.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/CellSetSingleType.h>

#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapField.h>

#include "VtkmSpheresOverlap.h"

using namespace vistle;

struct CountOverlapsWorklet: public vtkm::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn coords, ExecObject locator, FieldOut overlaps);
    using ExecutionSignature = void(InputIndex, _1, _2, _3);

    template<typename Point, typename PointLocatorExecObject>
    VTKM_EXEC void operator()(const vtkm::Id id, const Point &point, const PointLocatorExecObject &locator,
                              vtkm::Id &nrOverlaps) const
    {
        locator.CountOverlaps(id, point, nrOverlaps);
    }
};

struct CreateConnectionLinesWorklet: public vtkm::worklet::WorkletMapField {
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
        locator.CreateConnectionLines(id, point, visitId, connectivity, thickness);
    }
};

VTKM_CONT PointLocatorCellLists VtkmSpheresOverlap::CreateSearchGrid(const vtkm::cont::CoordinateSystem &coordinates,
                                                                     const vtkm::cont::Field &radii)
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

VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id>
VtkmSpheresOverlap::CountOverlapsPerPoint(const PointLocatorCellLists &pointLocator)
{
    vtkm::cont::ArrayHandle<vtkm::Id> result;
    this->Invoke(CountOverlapsWorklet{}, pointLocator.GetCoordinates(), &pointLocator, result);
    return result;
}

VTKM_CONT void VtkmSpheresOverlap::CreateConnectionLines(const PointLocatorCellLists &pointLocator,
                                                         const vtkm::cont::ArrayHandle<vtkm::Id> &overlapsPerPoint,
                                                         vtkm::cont::ArrayHandle<vtkm::Id> &linesConnectivity,
                                                         vtkm::cont::ArrayHandle<vtkm::FloatDefault> &linesThicknesses)
{
    // mapping is not 1-to-1 since a sphere can overlap with arbitrarily many other spheres (including none)
    auto spheresToLinesMapping = CreateConnectionLinesWorklet::MakeScatter(overlapsPerPoint);

    this->Invoke(CreateConnectionLinesWorklet{}, spheresToLinesMapping, pointLocator.GetCoordinates(), &pointLocator,
                 vtkm::cont::make_ArrayHandleGroupVec<2>(linesConnectivity), linesThicknesses);
}

VTKM_CONT vtkm::cont::DataSet CreateDataset(const vtkm::cont::CoordinateSystem &coordinates,
                                            const vtkm::cont::ArrayHandle<vtkm::Id> &linesConnectivity,
                                            const vtkm::cont::ArrayHandle<vtkm::FloatDefault> &linesThicknesses)
{
    vtkm::cont::CellSetSingleType<> linesCellSet;
    linesCellSet.Fill(linesConnectivity.GetNumberOfValues(), vtkm::CELL_SHAPE_LINE, 2, linesConnectivity);

    vtkm::cont::DataSet result;
    linesCellSet.GetNumberOfPoints();

    // make sure vtkm understands that this dataset is empty, if no overlaps are found
    if (linesConnectivity.GetNumberOfValues() > 0)
        result.AddCoordinateSystem(coordinates);

    result.SetCellSet(linesCellSet);
    result.AddCellField("lineThickness", linesThicknesses);
    return result;
}

VTKM_CONT vtkm::cont::DataSet VtkmSpheresOverlap::DoExecute(const vtkm::cont::DataSet &spheres)
{
    auto sphereCenters = spheres.GetCoordinateSystem();

    auto pointLocator = CreateSearchGrid(sphereCenters, spheres.GetPointField("radius"));
    auto overlapsPerPoint = CountOverlapsPerPoint(pointLocator);

    vtkm::cont::ArrayHandle<vtkm::Id> linesConnectivity;
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> linesThicknesses;
    CreateConnectionLines(pointLocator, overlapsPerPoint, linesConnectivity, linesThicknesses);

    return CreateDataset(sphereCenters, linesConnectivity, linesThicknesses);
}
