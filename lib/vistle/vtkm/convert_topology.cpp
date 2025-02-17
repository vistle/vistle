#include "convert_topology.h"
#include "convert_worklets.h"
#include "convert.h"

#include <vistle/core/points.h>
#include <vistle/core/lines.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/polygons.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>

#include <vistle/core/shm_array_impl.h>

#include <vtkm/cont/CellSetExplicit.h>


namespace vistle {

namespace {

template<typename Obj>
typename Obj::ptr toNgons(vtkm::cont::CellSetSingleType<> &scellset)
{
    auto connectivity = scellset.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    typename Obj::ptr ngons(new Obj(0, 0));
    ngons->d()->cl->setHandle(connectivity);
    return ngons;
}

template<typename Obj, typename ConnType, typename ElemType>
typename Obj::ptr toIndexed(ConnType &connectivity, ElemType &elements)
{
    typename Obj::ptr idx(new Obj(0, 0, 0));
    idx->d()->cl->setHandle(connectivity);
    idx->d()->el->setHandle(elements);
    return idx;
};

template<typename Obj>
typename Obj::ptr toIndexed(vtkm::cont::CellSetSingleType<> &scellset)
{
    auto connectivity = scellset.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    auto elements = scellset.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    return toIndexed<Obj>(connectivity, elements);
};

} // namespace

Object::ptr vtkmGetTopology(const vtkm::cont::DataSet &dataset)
{
    // get vertices that make up the dataset grid
    auto cellset = dataset.GetCellSet();

    // try conversion for uniform cell types first
    if (cellset.CanConvert<vtkm::cont::CellSetSingleType<>>()) {
        auto scellset = cellset.AsCellSet<vtkm::cont::CellSetSingleType<>>();

        if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_VERTEX) {
            Points::ptr points(new Points(Object::Initialized));
            return points;
        } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_LINE) {
            auto numPoints = dataset.GetNumberOfPoints();
            // get connectivity array of the dataset
            auto connectivity =
                scellset.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
            auto numConn = connectivity.GetNumberOfValues();
            auto numElem = numConn > 0 ? numConn / 2 : numPoints / 2;
            Lines::ptr lines(new Lines(numElem, 0, 0));
            lines->d()->cl->setHandle(connectivity);
            for (vtkm::Id index = 0; index < numElem; index++) {
                lines->el()[index] = 2 * index;
            }
            lines->el()[numElem] = numConn;
            return lines;
        } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_POLY_LINE) {
            return toIndexed<Lines>(scellset);
        } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE) {
            return toNgons<Triangles>(scellset);
        } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_QUAD) {
            return toNgons<Quads>(scellset);
        } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_POLYGON) {
            return toIndexed<Polygons>(scellset);
        }
    }

    if (cellset.CanConvert<vtkm::cont::CellSetExplicit<>>()) {
        auto ecellset = cellset.AsCellSet<vtkm::cont::CellSetExplicit<>>();
        auto elements = ecellset.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
        auto eshapes = ecellset.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
        auto econn = ecellset.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());

        const auto [mindim, maxdim] = getMinMaxDims(eshapes);

        if (mindim > maxdim) {
            // empty
        } else if (mindim == -1) {
            // unhanded cell type
        } else if (mindim != maxdim || maxdim == 3) {
            // require UnstructuredGrid for mixed cells
            UnstructuredGrid::ptr unstr(new UnstructuredGrid(0, 0, 0));
            unstr->d()->cl->setHandle(econn);
            unstr->d()->el->setHandle(elements);
            unstr->d()->tl->setHandle(eshapes);
            return unstr;
        } else if (mindim == 0) {
            Points::ptr points(new Points(Object::Initialized));
            return points;
        } else if (mindim == 1) {
            return toIndexed<Lines>(econn, elements);
        } else if (mindim == 2) {
            // all 2D cells representable as Polygons
            return toIndexed<Polygons>(econn, elements);
        }
    }

    return nullptr;
}

namespace {
template<class Obj>
struct ToShapeId;

template<>
struct ToShapeId<Triangles> {
    static constexpr vtkm::CellShapeIdEnum shape = vtkm::CELL_SHAPE_TRIANGLE;
    static constexpr int N = 3;
};
template<>
struct ToShapeId<Quads> {
    static constexpr vtkm::CellShapeIdEnum shape = vtkm::CELL_SHAPE_QUAD;
    static constexpr int N = 4;
};

template<>
struct ToShapeId<Lines> {
    static constexpr vtkm::CellShapeIdEnum shape = vtkm::CELL_SHAPE_POLY_LINE;
};
template<>
struct ToShapeId<Polygons> {
    static constexpr vtkm::CellShapeIdEnum shape = vtkm::CELL_SHAPE_POLYGON;
};

template<class Ngons>
ModuleStatusPtr fromNgons(vtkm::cont::DataSet &vtkmDataset, typename Ngons::const_ptr &ngon)
{
    auto numPoints = ngon->getNumCoords();
    auto numConn = ngon->getNumCorners();
    const auto shape = ToShapeId<Ngons>::shape;
    const auto N = ToShapeId<Ngons>::N;
    if (numConn > 0) {
        vtkm::cont::CellSetSingleType<> cellSet;
        auto conn = ngon->cl().handle();
        cellSet.Fill(numPoints, shape, N, conn);
        vtkmDataset.SetCellSet(cellSet);
    } else {
        vtkm::cont::CellSetSingleType<vtkm::cont::StorageTagCounting> cellSet;
        auto conn = vtkm::cont::make_ArrayHandleCounting(static_cast<vtkm::Id>(0), static_cast<vtkm::Id>(1), numPoints);
        cellSet.Fill(numPoints, shape, N, conn);
        vtkmDataset.SetCellSet(cellSet);
    }

    return Success();
};

template<class Idx>
ModuleStatusPtr fromIndexed(vtkm::cont::DataSet &vtkmDataset, typename Idx::const_ptr &idx)
{
    auto numPoints = idx->getNumCoords();
    auto numCells = idx->getNumElements();
    auto conn = idx->cl().handle();
    auto offs = idx->el().handle();
    auto shapesc = vtkm::cont::make_ArrayHandleConstant(vtkm::UInt8{ToShapeId<Idx>::shape}, numCells);
#ifdef VISTLE_VTKM_TYPES
    vtkm::cont::CellSetExplicit<typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag> cellSet;
    auto &shapes = shapesc;
#else
    vtkm::cont::UnknownArrayHandle shapescu(shapesc);
    auto shapesu = shapescu.NewInstanceBasic();
    shapesu.CopyShallowIfPossible(shapescu);
    vtkm::cont::ArrayHandleBasic<vtkm::UInt8> shapes;
    if (!shapesu.CanConvert<vtkm::cont::ArrayHandleBasic<vtkm::UInt8>>()) {
        return Error("Cannot convert shapes array to basic array handle.");
    }
    shapes = shapesu.AsArrayHandle<vtkm::cont::ArrayHandleBasic<vtkm::UInt8>>();
    vtkm::cont::CellSetExplicit<> cellSet;
#endif
    cellSet.Fill(numPoints, shapes, conn, offs);
    vtkmDataset.SetCellSet(cellSet);

    return Success();
}

} // namespace

ModuleStatusPtr vtkmSetTopology(vtkm::cont::DataSet &vtkmDataset, vistle::Object::const_ptr grid)
{
    if (auto str = grid->getInterface<StructuredGridBase>()) {
        vtkm::Id nx = str->getNumDivisions(0);
        vtkm::Id ny = str->getNumDivisions(1);
        vtkm::Id nz = str->getNumDivisions(2);
        if (nz > 0) {
            vtkm::cont::CellSetStructured<3> str3;
            str3.SetPointDimensions({nx, ny, nz});
            vtkmDataset.SetCellSet(str3);
        } else if (ny > 0) {
            vtkm::cont::CellSetStructured<2> str2;
            str2.SetPointDimensions({ny, nx});
            vtkmDataset.SetCellSet(str2);
        } else {
            vtkm::cont::CellSetStructured<1> str1;
            str1.SetPointDimensions(nx);
            vtkmDataset.SetCellSet(str1);
        }
    } else if (auto unstructuredGrid = UnstructuredGrid::as(grid)) {
        auto numPoints = unstructuredGrid->getNumCoords();

        auto conn = unstructuredGrid->cl().handle();
        auto offs = unstructuredGrid->el().handle();
        auto shapes = unstructuredGrid->tl().handle();

        vtkm::cont::CellSetExplicit<> cellSetExplicit;
        cellSetExplicit.Fill(numPoints, shapes, conn, offs);

        // create vtkm dataset
        vtkmDataset.SetCellSet(cellSetExplicit);
    } else if (auto tri = Triangles::as(grid)) {
        return fromNgons<Triangles>(vtkmDataset, tri);
    } else if (auto quads = Quads::as(grid)) {
        return fromNgons<Quads>(vtkmDataset, quads);
    } else if (auto poly = Polygons::as(grid)) {
        return fromIndexed<Polygons>(vtkmDataset, poly);
    } else if (auto line = Lines::as(grid)) {
        return fromIndexed<Lines>(vtkmDataset, line);
    } else if (auto point = Points::as(grid)) {
        auto numPoints = point->getNumPoints();
        vtkm::cont::CellSetSingleType<vtkm::cont::StorageTagIndex> cellSet;
        auto conn = vtkm::cont::make_ArrayHandleIndex(numPoints);
        cellSet.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, conn);
        vtkmDataset.SetCellSet(cellSet);
    } else {
        return Error("Encountered unsupported grid type while attempting to convert Vistle grid to VTK-m dataset.");
    }

    return Success();
}

} // namespace vistle
