#include <vtkm/cont/Algorithm.h>

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
