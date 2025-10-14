#include "convert_topology.h"
#include "convert_worklets.h"
#include "convert.h"

#include <vistle/core/celltypes.h>
#include <vistle/core/points.h>
#include <vistle/core/lines.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/polygons.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/layergrid.h>

#include <vistle/core/shm_array_impl.h>

#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include "ArrayHandleCountingModulus.h"


namespace vistle {

template<typename StorageType>
bool containsPolyhedralCells(const viskores::cont::CellSetSingleType<StorageType> &cellset)
{
    return cellset.GetNumberOfCells() > 0 && cellset.GetCellShape(0) == vistle::cell::CellType::POLYHEDRON;
}

template<typename StorageType>
bool containsPolyhedralCells(const viskores::cont::CellSetExplicit<StorageType> &cellset)
{
    auto shapes = cellset.GetShapesArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
    const auto [mindim, maxdim] = getMinMaxDims(shapes);

    return mindim == -2;
}


namespace {

template<typename Obj>
typename Obj::ptr toNgons(viskores::cont::CellSetSingleType<> &scellset)
{
    auto connectivity =
        scellset.GetConnectivityArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
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
typename Obj::ptr toIndexed(viskores::cont::CellSetSingleType<> &scellset)
{
    auto connectivity =
        scellset.GetConnectivityArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
    auto elements = scellset.GetOffsetsArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
    return toIndexed<Obj>(connectivity, elements);
};

} // namespace

Object::ptr vtkmGetTopology(const viskores::cont::DataSet &dataset)
{
    // get vertices that make up the dataset grid
    auto cellset = dataset.GetCellSet();
    auto uPointCoordinates = dataset.GetCoordinateSystem().GetData();
    viskores::cont::UnknownArrayHandle unknown(uPointCoordinates);

    // first try structured grids
    bool isUniform = unknown.CanConvert<viskores::cont::ArrayHandleUniformPointCoordinates>();
    bool isLayered = unknown.CanConvert<AHLG<vistle::Scalar>>();
    bool isCartesian = false;
    bool isStructured = false;
    vistle::Index ncells[3] = {0, 0, 0};

    if (cellset.CanConvert<viskores::cont::CellSetStructured<1>>()) {
        isStructured = true;
        auto scellset = cellset.AsCellSet<viskores::cont::CellSetStructured<1>>();
        auto d = scellset.GetCellDimensions();
        ncells[0] = d;
    } else if (cellset.CanConvert<viskores::cont::CellSetStructured<2>>()) {
        isStructured = true;
        auto scellset = cellset.AsCellSet<viskores::cont::CellSetStructured<2>>();
        auto d = scellset.GetCellDimensions();
        for (int i = 0; i < 2; ++i)
            ncells[i] = d[i];
        isCartesian = unknown.CanConvert<AHCP<vistle::Scalar>>();
    } else if (cellset.CanConvert<viskores::cont::CellSetStructured<3>>()) {
        isStructured = true;
        auto scellset = cellset.AsCellSet<viskores::cont::CellSetStructured<3>>();
        auto d = scellset.GetCellDimensions();
        for (int i = 0; i < 3; ++i)
            ncells[i] = d[i];
        isCartesian = unknown.CanConvert<AHCP<vistle::Scalar>>();
    }

    if (isStructured && isUniform) {
        return std::make_shared<vistle::UniformGrid>(ncells[0] + 1, ncells[1] + 1, ncells[2] + 1);
    } else if (isStructured && isCartesian) {
        return std::make_shared<vistle::RectilinearGrid>(ncells[0] + 1, ncells[1] + 1, ncells[2] + 1);
    } else if (isStructured && isLayered) {
        // FIXME: still need to find a way to get at the individual components of an ArrayHandleExtractComponent
        //return std::make_shared<vistle::LayerGrid>(ncells[0] + 1, ncells[1] + 1, ncells[2] + 1);
        return std::make_shared<vistle::StructuredGrid>(ncells[0] + 1, ncells[1] + 1, ncells[2] + 1);
    } else if (isStructured) {
        return std::make_shared<vistle::StructuredGrid>(ncells[0] + 1, ncells[1] + 1, ncells[2] + 1);
    }

    // try conversion for uniform cell types first
    if (cellset.CanConvert<viskores::cont::CellSetSingleType<viskores::cont::StorageTagIndex>>()) {
        auto scellset = cellset.AsCellSet<viskores::cont::CellSetSingleType<viskores::cont::StorageTagIndex>>();
        if (cellset.GetCellShape(0) == viskores::CELL_SHAPE_VERTEX) {
            Points::ptr points(new Points(Object::Initialized));
            return points;
        }
    }

    // try conversion for uniform cell types first
    if (cellset.CanConvert<viskores::cont::CellSetSingleType<>>()) {
        auto scellset = cellset.AsCellSet<viskores::cont::CellSetSingleType<>>();

        if (cellset.GetCellShape(0) == viskores::CELL_SHAPE_VERTEX) {
            Points::ptr points(new Points(Object::Initialized));
            return points;
        } else if (cellset.GetCellShape(0) == viskores::CELL_SHAPE_LINE) {
            auto numPoints = dataset.GetNumberOfPoints();
            // get connectivity array of the dataset
            auto connectivity =
                scellset.GetConnectivityArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
            auto numConn = connectivity.GetNumberOfValues();
            auto numElem = numConn > 0 ? numConn / 2 : numPoints / 2;
            Lines::ptr lines(new Lines(numElem, 0, 0));
            lines->d()->cl->setHandle(connectivity);
            for (viskores::Id index = 0; index < numElem; index++) {
                lines->el()[index] = 2 * index;
            }
            lines->el()[numElem] = numConn;
            return lines;
        } else if (cellset.GetCellShape(0) == viskores::CELL_SHAPE_POLY_LINE) {
            return toIndexed<Lines>(scellset);
        } else if (cellset.GetCellShape(0) == viskores::CELL_SHAPE_TRIANGLE) {
            return toNgons<Triangles>(scellset);
        } else if (cellset.GetCellShape(0) == viskores::CELL_SHAPE_QUAD) {
            return toNgons<Quads>(scellset);
        } else if (cellset.GetCellShape(0) == viskores::CELL_SHAPE_POLYGON) {
            return toIndexed<Polygons>(scellset);
        } else {
            std::cerr << "ERROR vtkmGetTopology: encountered unsupported cell type (" << cellset.GetCellShape(0) << ")"
                      << std::endl;
            return nullptr;
        }
    }

    //arbitrary unstructured grids with mixed cell types
    if (cellset.CanConvert<viskores::cont::CellSetExplicit<>>()) {
        auto ecellset = cellset.AsCellSet<viskores::cont::CellSetExplicit<>>();
        auto elements =
            ecellset.GetOffsetsArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
        auto eshapes = ecellset.GetShapesArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
        auto econn =
            ecellset.GetConnectivityArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());

        const auto [mindim, maxdim] = getMinMaxDims(eshapes);

        if (mindim > maxdim) {
            std::cerr << "vtkmGetTopology: empty cell set" << std::endl;
        } else if (mindim == -2) {
            std::cerr << "ERROR vtkmGetTopology: polyhedral cells are not supported!" << std::endl;
        } else if (mindim == -3) {
            std::cerr << "ERROR vtkmGetTopology: encountered unsupported cell type!" << std::endl;
        } else if (mindim != maxdim || maxdim == 3 || mindim == -1) {
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
    static constexpr viskores::CellShapeIdEnum shape = viskores::CELL_SHAPE_TRIANGLE;
    static constexpr int N = 3;
};
template<>
struct ToShapeId<Quads> {
    static constexpr viskores::CellShapeIdEnum shape = viskores::CELL_SHAPE_QUAD;
    static constexpr int N = 4;
};

template<>
struct ToShapeId<Lines> {
    static constexpr viskores::CellShapeIdEnum shape = viskores::CELL_SHAPE_POLY_LINE;
};
template<>
struct ToShapeId<Polygons> {
    static constexpr viskores::CellShapeIdEnum shape = viskores::CELL_SHAPE_POLYGON;
};

template<class Ngons>
ModuleStatusPtr fromNgons(viskores::cont::DataSet &vtkmDataset, typename Ngons::const_ptr &ngon)
{
    auto numPoints = ngon->getNumCoords();
    auto numConn = ngon->getNumCorners();
    const auto shape = ToShapeId<Ngons>::shape;
    const auto N = ToShapeId<Ngons>::N;
    if (numConn > 0) {
        viskores::cont::CellSetSingleType<> cellSet;
        auto conn = ngon->cl().handle();
        cellSet.Fill(numPoints, shape, N, conn);

        if (containsPolyhedralCells(cellSet))
            return UnsupportedCellTypeError(true);

        vtkmDataset.SetCellSet(cellSet);
    } else {
        viskores::cont::CellSetSingleType<viskores::cont::StorageTagCounting> cellSet;
        auto conn = viskores::cont::make_ArrayHandleCounting(static_cast<viskores::Id>(0), static_cast<viskores::Id>(1),
                                                             numPoints);
        cellSet.Fill(numPoints, shape, N, conn);

        if (containsPolyhedralCells(cellSet))
            return UnsupportedCellTypeError(true);

        vtkmDataset.SetCellSet(cellSet);
    }

    return Success();
};

template<class Idx>
ModuleStatusPtr fromIndexed(viskores::cont::DataSet &vtkmDataset, typename Idx::const_ptr &idx)
{
    auto numPoints = idx->getNumCoords();
    auto numCells = idx->getNumElements();
    auto conn = idx->cl().handle();
    auto offs = idx->el().handle();
    auto shapesc = viskores::cont::make_ArrayHandleConstant(viskores::UInt8{ToShapeId<Idx>::shape}, numCells);
#ifdef VISTLE_VISKORES_TYPES
    viskores::cont::CellSetExplicit<typename viskores::cont::ArrayHandleConstant<viskores::UInt8>::StorageTag> cellSet;
    auto &shapes = shapesc;
#else
    viskores::cont::UnknownArrayHandle shapescu(shapesc);
    auto shapesu = shapescu.NewInstanceBasic();
    shapesu.CopyShallowIfPossible(shapescu);
    viskores::cont::ArrayHandleBasic<viskores::UInt8> shapes;
    if (!shapesu.CanConvert<viskores::cont::ArrayHandleBasic<viskores::UInt8>>()) {
        return Error("Cannot convert shapes array to basic array handle.");
    }
    shapes = shapesu.AsArrayHandle<viskores::cont::ArrayHandleBasic<viskores::UInt8>>();
    viskores::cont::CellSetExplicit<> cellSet;
#endif
    cellSet.Fill(numPoints, shapes, conn, offs);

    if (containsPolyhedralCells(cellSet))
        return UnsupportedCellTypeError(true);

    vtkmDataset.SetCellSet(cellSet);

    return Success();
}

} // namespace

ModuleStatusPtr vtkmSetTopology(viskores::cont::DataSet &vtkmDataset, vistle::Object::const_ptr grid)
{
    if (auto str = grid->getInterface<StructuredGridBase>()) {
        viskores::Id nx = str->getNumDivisions(0);
        viskores::Id ny = str->getNumDivisions(1);
        viskores::Id nz = str->getNumDivisions(2);
        if (nz > 0) {
            viskores::cont::CellSetStructured<3> str3;
            str3.SetPointDimensions({nx, ny, nz});
            vtkmDataset.SetCellSet(str3);
        } else if (ny > 0) {
            viskores::cont::CellSetStructured<2> str2;
            str2.SetPointDimensions({ny, nx});
            vtkmDataset.SetCellSet(str2);
        } else {
            viskores::cont::CellSetStructured<1> str1;
            str1.SetPointDimensions(nx);
            vtkmDataset.SetCellSet(str1);
        }
    } else if (auto unstructuredGrid = UnstructuredGrid::as(grid)) {
        auto numPoints = unstructuredGrid->getNumCoords();

        auto conn = unstructuredGrid->cl().handle();
        auto offs = unstructuredGrid->el().handle();
        auto shapes = unstructuredGrid->tl().handle();

        viskores::cont::CellSetExplicit<> cellSetExplicit;
        cellSetExplicit.Fill(numPoints, shapes, conn, offs);

        if (containsPolyhedralCells(cellSetExplicit))
            return UnsupportedCellTypeError(true);

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
        viskores::cont::CellSetSingleType<viskores::cont::StorageTagIndex> cellSet;
        auto conn = viskores::cont::make_ArrayHandleIndex(numPoints);
        cellSet.Fill(numPoints, viskores::CELL_SHAPE_VERTEX, 1, conn);

        if (containsPolyhedralCells(cellSet))
            return UnsupportedCellTypeError(true);

        vtkmDataset.SetCellSet(cellSet);
    } else {
        return Error("Encountered unsupported grid type while attempting to convert Vistle grid to Viskores dataset.");
    }

    return Success();
}

} // namespace vistle
