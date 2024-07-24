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

struct ReorderArray: vtkm::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn order, WholeArrayIn arrayToReorder, FieldOut reordered);
    using ExecutionSignature = void(_1, _2, _3);

    template<typename PortalType, typename OutputType>
    VTKM_EXEC void operator()(const vtkm::Id &newIndex, PortalType toReorder, OutputType &output) const
    {
        output = toReorder.Get(newIndex);
    }
};

/*
    Vistle and vtk-m store the point cordinates which make up a cellset consisting solely of lines
    differently: Vtk-m creates a list containing each point once, while Vistle stores all pairs which
    make up each line in a list (this leads to duplicates in the coordinates).

    TODO: Find out if we can avoid this?
*/
void linesCoordinatesToVistle(const vtkm::cont::UnknownArrayHandle &coordinates,
                              vtkm::cont::ArrayHandle<vtkm::Id> connectivity, vistle::Lines::ptr lines)
{
    vtkm::cont::Invoker invoke;

    if (coordinates.CanConvert<vtkm::cont::ArrayHandle<vtkm::Vec<vistle::Scalar, 3>>>()) {
        auto typedCoordinates = coordinates.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec<vistle::Scalar, 3>>>();
        for (int d = 0; d < 3; ++d) {
            vtkm::cont::ArrayHandle<vtkm::FloatDefault> vistleCoordinates;
            invoke(ReorderArray{}, connectivity, make_ArrayHandleExtractComponent(typedCoordinates, d),
                   vistleCoordinates);
            lines->d()->x[d]->setHandle(vistleCoordinates);
        }

    } else if (coordinates.CanConvert<vtkm::cont::ArrayHandleSOA<vtkm::Vec3f>>()) {
        auto typedCoordinates = coordinates.AsArrayHandle<vtkm::cont::ArrayHandleSOA<vtkm::Vec3f>>();
        for (int d = 0; d < 3; ++d) {
            vtkm::cont::ArrayHandle<vtkm::FloatDefault> vistleCoordinates;
            invoke(ReorderArray{}, connectivity, typedCoordinates.GetArray(d), vistleCoordinates);
            lines->d()->x[d]->setHandle(vistleCoordinates);
        }

    } else {
        std::cerr << "VTKm coordinate system uses unsupported array handle storage." << std::endl;
    }
}
