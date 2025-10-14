#ifndef VISTLE_VTKM_CONVERT_WORKLETS_H
#define VISTLE_VTKM_CONVERT_WORKLETS_H

#include <viskores/cont/Algorithm.h>

#include <viskores/worklet/WorkletMapField.h>

#include <vistle/core/celltypes.h>


struct ShapeToDimensionWorklet: viskores::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn shapes, FieldOut dimensions);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;

    template<typename InputType, typename OutputType>
    VISKORES_EXEC void operator()(const InputType &shape, OutputType &dim) const
    {
        dim = shape;
        switch (shape) {
        case viskores::CELL_SHAPE_VERTEX:
            dim = 0;
            break;
        case viskores::CELL_SHAPE_LINE:
        case viskores::CELL_SHAPE_POLY_LINE:
            dim = 1;
            break;
        case viskores::CELL_SHAPE_TRIANGLE:
        case viskores::CELL_SHAPE_QUAD:
        case viskores::CELL_SHAPE_POLYGON:
            dim = 2;
            break;
        case viskores::CELL_SHAPE_TETRA:
        case viskores::CELL_SHAPE_HEXAHEDRON:
        case viskores::CELL_SHAPE_WEDGE:
        case viskores::CELL_SHAPE_PYRAMID:
            dim = 3;
            break;
        case viskores::CELL_SHAPE_EMPTY:
            dim = -1;
            break;
        case vistle::cell::CellType::POLYHEDRON:
            dim = -2; // supported by Vistle, but not by Viskores
            break;
        default:
            dim = -3; // unsupported
            break;
        }
    }
};

template<typename StorageType>
std::tuple<int, int> getMinMaxDims(const viskores::cont::ArrayHandle<viskores::UInt8, StorageType> &shapes)
{
    viskores::cont::Invoker invoke;
    viskores::cont::ArrayHandle<viskores::Int8> dims;
    invoke(ShapeToDimensionWorklet{}, shapes, dims);
    return {viskores::cont::Algorithm::Reduce(dims, 10, viskores::Minimum()),
            viskores::cont::Algorithm::Reduce(dims, -10, viskores::Maximum())};
}

#endif
