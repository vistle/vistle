#include <vtkm/cont/Algorithm.h>
// TODO: remove
#include <vtkm/cont/ArrayHandleExtractComponent.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/DataSet.h>
#include <vistle/core/lines.h>
#include <vistle/core/shm_array_impl.h>

#include <vtkm/worklet/WorkletMapField.h>

#include "convert_worklets.h"

struct ShapeToDimensionWorklet: vtkm::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn shapes, FieldOut dimensions);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;

    template<typename InputType, typename OutputType>
    VTKM_EXEC void operator()(const InputType &shape, OutputType &dim) const
    {
        dim = shape;
        switch (shape) {
        case vtkm::CELL_SHAPE_VERTEX:
            dim = 0;
            break;
        case vtkm::CELL_SHAPE_LINE:
        case vtkm::CELL_SHAPE_POLY_LINE:
            dim = 1;
            break;
        case vtkm::CELL_SHAPE_TRIANGLE:
        case vtkm::CELL_SHAPE_QUAD:
        case vtkm::CELL_SHAPE_POLYGON:
            dim = 2;
            break;
        case vtkm::CELL_SHAPE_TETRA:
        case vtkm::CELL_SHAPE_HEXAHEDRON:
        case vtkm::CELL_SHAPE_WEDGE:
        case vtkm::CELL_SHAPE_PYRAMID:
            dim = 3;
            break;
        default:
            dim = -1;
            break;
        }
    }
};

std::tuple<int, int> getMinMaxDims(vtkm::cont::ArrayHandle<vtkm::UInt8> &shapes)
{
    vtkm::cont::Invoker invoke;
    vtkm::cont::ArrayHandle<vtkm::Int8> dims;
    invoke(ShapeToDimensionWorklet{}, shapes, dims);
    return {vtkm::cont::Algorithm::Reduce(dims, 10, vtkm::Minimum()),
            vtkm::cont::Algorithm::Reduce(dims, -1, vtkm::Maximum())};
}

struct LinesCoordinatesToVistle: vtkm::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn connectivity, WholeArrayIn coordIn, FieldOut coordOut);
    using ExecutionSignature = void(_1, _2, _3);

    template<typename ConnType, typename CoordPortal, typename OutputType>
    VTKM_EXEC void operator()(const ConnType &index, CoordPortal inputCoords, OutputType &outputCoord) const
    {
        outputCoord = inputCoords.Get(index);
    }
};

void linesCoordinatesToVistle(const vtkm::cont::UnknownArrayHandle &unknown,
                              vtkm::cont::ArrayHandle<vtkm::Id> connectivity, vistle::Lines::ptr lines)
{
    vtkm::cont::Invoker invoke;

    //auto unknown = dataset.GetCoordinateSystem().GetData();
    if (unknown.CanConvert<vtkm::cont::ArrayHandle<vtkm::Vec<vistle::Scalar, 3>>>()) {
        auto vtkmCoord = unknown.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec<vistle::Scalar, 3>>>();
        for (int d = 0; d < 3; ++d) {
            vtkm::cont::ArrayHandle<vtkm::FloatDefault> lineCoord;
            auto coordComponent = make_ArrayHandleExtractComponent(vtkmCoord, d);
            invoke(LinesCoordinatesToVistle{}, connectivity, coordComponent, lineCoord);
            lines->d()->x[d]->setHandle(lineCoord);
        }

    } else if (unknown.CanConvert<vtkm::cont::ArrayHandleSOA<vtkm::Vec3f>>()) {
        auto vtkmCoord = unknown.AsArrayHandle<vtkm::cont::ArrayHandleSOA<vtkm::Vec3f>>();
        for (int d = 0; d < 3; ++d) {
            vtkm::cont::ArrayHandle<vtkm::FloatDefault> lineCoord;
            auto coordComponent = vtkmCoord.GetArray(d);

            invoke(LinesCoordinatesToVistle{}, connectivity, coordComponent, lineCoord);
            auto myPortal = lineCoord.ReadPortal();
            lines->d()->x[d]->setHandle(lineCoord);
        }

    } else {
        std::cerr << "VTKm coordinate system uses unsupported array handle storage." << std::endl;
    }
}
