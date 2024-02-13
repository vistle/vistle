#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/CellSetExplicit.h>

#include <vistle/core/scalars.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>

#include <vistle/core/points.h>
#include <vistle/core/lines.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/polygons.h>

#include <boost/mpl/for_each.hpp>

#include "convert.h"


namespace vistle {

// for testing also version that returns std::vector instead of array handle
std::vector<vtkm::UInt8> vistleTypeListToVtkmShapesVector(Index numElements, const Byte *typeList)
{
    std::vector<vtkm::UInt8> shapes(numElements);
    for (unsigned int i = 0; i < shapes.size(); i++) {
        if (typeList[i] == cell::POINT)
            shapes[i] = vtkm::CELL_SHAPE_VERTEX;
        else if (typeList[i] == cell::BAR)
            shapes[i] = vtkm::CELL_SHAPE_LINE;
        else if (typeList[i] == cell::TRIANGLE)
            shapes[i] = vtkm::CELL_SHAPE_TRIANGLE;
        else if (typeList[i] == cell::POLYGON)
            shapes[i] = vtkm::CELL_SHAPE_POLYGON;
        else if (typeList[i] == cell::QUAD)
            shapes[i] = vtkm::CELL_SHAPE_QUAD;
        else if (typeList[i] == cell::TETRAHEDRON)
            shapes[i] = vtkm::CELL_SHAPE_TETRA;
        else if (typeList[i] == cell::HEXAHEDRON)
            shapes[i] = vtkm::CELL_SHAPE_HEXAHEDRON;
        else if (typeList[i] == cell::PRISM)
            shapes[i] = vtkm::CELL_SHAPE_WEDGE;
        else if (typeList[i] == cell::PYRAMID)
            shapes[i] = vtkm::CELL_SHAPE_PYRAMID;
        else {
            shapes.clear();
            break;
        }
    }
    return shapes;
}

VtkmTransformStatus vtkmSetGrid(vtkm::cont::DataSet &vtkmDataset, vistle::Object::const_ptr grid)
{
    // swap x and z coordinates to account for different indexing in structured data
    if (auto coords = Coords::as(grid)) {
        auto xCoords = coords->x();
        auto yCoords = coords->y();
        auto zCoords = coords->z();

        auto coordinateSystem = vtkm::cont::CoordinateSystem(
            "coordinate system",
            vtkm::cont::make_ArrayHandleSOA(coords->z().handle(), coords->y().handle(), coords->x().handle()));

        vtkmDataset.AddCoordinateSystem(coordinateSystem);

        if (coords->normals()) {
            auto normals = coords->normals();
            vtkmAddField(vtkmDataset, normals, "normals");
        }
    } else if (auto uni = UniformGrid::as(grid)) {
        auto nx = uni->getNumDivisions(0);
        auto ny = uni->getNumDivisions(1);
        auto nz = uni->getNumDivisions(2);
        const auto *min = uni->min(), *dist = uni->dist();
        vtkm::cont::ArrayHandleUniformPointCoordinates uniformCoordinates(
            vtkm::Id3(nz, ny, nx), vtkm::Vec3f{min[2], min[1], min[0]}, vtkm::Vec3f{dist[2], dist[1], dist[0]});
        auto coordinateSystem = vtkm::cont::CoordinateSystem("uniform", uniformCoordinates);
        vtkmDataset.AddCoordinateSystem(coordinateSystem);
    } else if (auto rect = RectilinearGrid::as(grid)) {
        auto xc = rect->coords(0).handle();
        auto yc = rect->coords(1).handle();
        auto zc = rect->coords(2).handle();

        vtkm::cont::ArrayHandleCartesianProduct rectilinearCoordinates(zc, yc, xc);
        auto coordinateSystem = vtkm::cont::CoordinateSystem("rectilinear", rectilinearCoordinates);
        vtkmDataset.AddCoordinateSystem(coordinateSystem);
    } else {
        return VtkmTransformStatus::UNSUPPORTED_GRID_TYPE;
    }

    auto indexedGrid = Indexed::as(grid);

    if (auto str = grid->getInterface<StructuredGridBase>()) {
        vtkm::Id nx = str->getNumDivisions(0);
        vtkm::Id ny = str->getNumDivisions(1);
        vtkm::Id nz = str->getNumDivisions(2);
        if (nz > 0) {
            vtkm::cont::CellSetStructured<3> str3;
            str3.SetPointDimensions({nz, ny, nx});
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
    } else if (auto unstructuredGrid = UnstructuredGrid::as(indexedGrid)) {
        auto numPoints = indexedGrid->getNumCoords();

        auto conn = indexedGrid->cl().handle();
        auto offs = indexedGrid->el().handle();
        auto shapes = unstructuredGrid->tl().handle();

        vtkm::cont::CellSetExplicit<> cellSetExplicit;
        cellSetExplicit.Fill(numPoints, shapes, conn, offs);

        // create vtkm dataset
        vtkmDataset.SetCellSet(cellSetExplicit);
    } else if (auto tri = Triangles::as(grid)) {
        auto numPoints = tri->getNumCoords();
        auto numConn = tri->getNumCorners();
        auto numCells = tri->getNumElements();
        if (numConn > 0) {
            vtkm::cont::CellSetSingleType<> cellSet;
            auto conn = tri->cl().handle();
            cellSet.Fill(numPoints, vtkm::CELL_SHAPE_TRIANGLE, 3, conn);
            vtkmDataset.SetCellSet(cellSet);
        } else {
            vtkm::cont::CellSetSingleType<vtkm::cont::StorageTagCounting> cellSet;
            auto conn =
                vtkm::cont::make_ArrayHandleCounting(static_cast<vtkm::Id>(0), static_cast<vtkm::Id>(3), numCells);
            cellSet.Fill(numPoints, vtkm::CELL_SHAPE_TRIANGLE, 3, conn);
            vtkmDataset.SetCellSet(cellSet);
        }
    } else if (auto quads = Quads::as(grid)) {
        auto numPoints = quads->getNumCoords();
        auto numConn = quads->getNumCorners();
        auto numCells = quads->getNumElements();
        if (numConn > 0) {
            vtkm::cont::CellSetSingleType<> cellSet;
            auto conn = quads->cl().handle();
            cellSet.Fill(numPoints, vtkm::CELL_SHAPE_QUAD, 4, conn);
            vtkmDataset.SetCellSet(cellSet);
        } else {
            vtkm::cont::CellSetSingleType<vtkm::cont::StorageTagCounting> cellSet;
            auto conn =
                vtkm::cont::make_ArrayHandleCounting(static_cast<vtkm::Id>(0), static_cast<vtkm::Id>(4), numCells);
            cellSet.Fill(numPoints, vtkm::CELL_SHAPE_QUAD, 4, conn);
            vtkmDataset.SetCellSet(cellSet);
        }
    } else if (auto poly = Polygons::as(grid)) {
        poly->check();
        auto numPoints = poly->getNumCoords();
        auto numCells = poly->getNumElements();
        auto conn = poly->cl().handle();
        auto offs = poly->el().handle();
        auto shapesc = vtkm::cont::make_ArrayHandleConstant(vtkm::UInt8{vtkm::CELL_SHAPE_POLYGON}, numCells);
#ifdef VISTLE_VTKM_TYPES
        vtkm::cont::CellSetExplicit<typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag> cellSet;
        auto &shapes = shapesc;
#else
        vtkm::cont::UnknownArrayHandle shapescu(shapesc);
        auto shapesu = shapescu.NewInstanceBasic();
        shapesu.CopyShallowIfPossible(shapescu);
        vtkm::cont::ArrayHandleBasic<vtkm::UInt8> shapes;
        if (!shapesu.CanConvert<vtkm::cont::ArrayHandleBasic<vtkm::UInt8>>()) {
            std::cerr << "cannot convert to basic array handle" << std::endl;
            return VtkmTransformStatus::UNSUPPORTED_GRID_TYPE;
        }
        shapes = shapesu.AsArrayHandle<vtkm::cont::ArrayHandleBasic<vtkm::UInt8>>();
        vtkm::cont::CellSetExplicit<> cellSet;
#endif
        cellSet.Fill(numPoints, shapes, conn, offs);
        vtkmDataset.SetCellSet(cellSet);
    } else if (auto line = Lines::as(grid)) {
        line->check();
        auto numPoints = line->getNumCoords();
        auto numCells = line->getNumElements();
        auto conn = line->cl().handle();
        auto offs = line->el().handle();
        auto shapesc = vtkm::cont::make_ArrayHandleConstant(vtkm::UInt8{vtkm::CELL_SHAPE_POLY_LINE}, numCells);
#ifdef VISTLE_VTKM_TYPES
        vtkm::cont::CellSetExplicit<typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag> cellSet;
        auto &shapes = shapesc;
#else
        vtkm::cont::UnknownArrayHandle shapescu(shapesc);
        auto shapesu = shapescu.NewInstanceBasic();
        shapesu.CopyShallowIfPossible(shapescu);
        vtkm::cont::ArrayHandleBasic<vtkm::UInt8> shapes;
        if (!shapesu.CanConvert<vtkm::cont::ArrayHandleBasic<vtkm::UInt8>>()) {
            std::cerr << "cannot convert to basic array handle" << std::endl;
            return VtkmTransformStatus::UNSUPPORTED_GRID_TYPE;
        }
        shapes = shapesu.AsArrayHandle<vtkm::cont::ArrayHandleBasic<vtkm::UInt8>>();
        vtkm::cont::CellSetExplicit<> cellSet;
#endif
        cellSet.Fill(numPoints, shapes, conn, offs);
        vtkmDataset.SetCellSet(cellSet);
    } else if (auto point = Points::as(grid)) {
        auto numPoints = point->getNumPoints();
        vtkm::cont::CellSetSingleType<vtkm::cont::StorageTagIndex> cellSet;
        auto conn = vtkm::cont::make_ArrayHandleIndex(numPoints);
        cellSet.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, conn);
        vtkmDataset.SetCellSet(cellSet);
    } else {
        return VtkmTransformStatus::UNSUPPORTED_GRID_TYPE;
    }

    vtkmDataset.SetGhostCellFieldName("ghost");
    if (indexedGrid) {
        if (indexedGrid->ghost().size() > 0) {
            std::cerr << "indexed: have ghost cells" << std::endl;
            auto ghost = indexedGrid->ghost().handle();
            vtkmDataset.SetGhostCellField("ghost", ghost);
        }
    } else if (auto tri = Triangles::as(grid)) {
        if (tri->ghost().size() > 0) {
            auto ghost = tri->ghost().handle();
            vtkmDataset.SetGhostCellField("ghost", ghost);
        }
    } else if (auto quad = Quads::as(grid)) {
        if (quad->ghost().size() > 0) {
            auto ghost = quad->ghost().handle();
            vtkmDataset.SetGhostCellField("ghost", ghost);
        }
    }

    return VtkmTransformStatus::SUCCESS;
}

struct AddField {
    vtkm::cont::DataSet &dataset;
    const DataBase::const_ptr &object;
    const std::string &name;
    bool &handled;
    AddField(vtkm::cont::DataSet &ds, const DataBase::const_ptr &obj, const std::string &name, bool &handled)
    : dataset(ds), object(obj), name(name), handled(handled)
    {}
    template<typename S>
    void operator()(S)
    {
        typedef Vec<S, 1> V1;
        //typedef Vec<S, 2> V2;
        typedef Vec<S, 3> V3;
        //typedef Vec<S, 4> V4;

        vtkm::cont::UnknownArrayHandle ah;
        if (auto in = V1::as(object)) {
            ah = in->x().handle();
        } else if (auto in = V3::as(object)) {
            auto ax = in->x().handle();
            auto ay = in->y().handle();
            auto az = in->z().handle();
            ah = vtkm::cont::make_ArrayHandleSOA(az, ay, ax);
        } else {
            return;
        }

        auto mapping = object->guessMapping();
        if (mapping == vistle::DataBase::Vertex) {
            dataset.AddPointField(name, ah);
        } else {
            dataset.AddCellField(name, ah);
        }

        handled = true;
    }
};


VtkmTransformStatus vtkmAddField(vtkm::cont::DataSet &vtkmDataSet, const vistle::DataBase::const_ptr &field,
                                 const std::string &name)
{
    bool handled = false;
    boost::mpl::for_each<Scalars>(AddField(vtkmDataSet, field, name, handled));
    if (handled)
        return VtkmTransformStatus::SUCCESS;

    return VtkmTransformStatus::UNSUPPORTED_FIELD_TYPE;
}


Object::ptr vtkmGetGeometry(vtkm::cont::DataSet &dataset)
{
    Object::ptr result;

    // get vertices that make up the dataset grid
    auto uPointCoordinates = dataset.GetCoordinateSystem().GetData();

    // we expect point coordinates to be stored as vtkm::Vec3 array handle
    assert((uPointCoordinates.CanConvert<vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>>() == true));

    auto pointCoordinates =
        uPointCoordinates.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>>();
    auto pointsPortal = pointCoordinates.ReadPortal();
    auto numPoints = dataset.GetNumberOfPoints();

    auto cellset = dataset.GetCellSet();

    // try conversion for uniform cell types first
    if (cellset.CanConvert<vtkm::cont::CellSetSingleType<>>()) {
        auto isoGrid = cellset.AsCellSet<vtkm::cont::CellSetSingleType<>>();
        // get connectivity array of the dataset
        auto connectivity =
            isoGrid.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
        auto connPortal = connectivity.ReadPortal();
        auto numConn = connectivity.GetNumberOfValues();
        auto numElem = cellset.GetNumberOfCells();

        if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_VERTEX) {
            Points::ptr points(new Points(numPoints));
            result = points;
        } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_LINE) {
            auto numElem = numConn > 0 ? numConn / 2 : numPoints / 2;
            Lines::ptr lines(new Lines(numElem, numConn, numPoints));
            for (vtkm::Id index = 0; index < numConn; index++) {
                lines->cl()[index] = connPortal.Get(index);
            }
            for (vtkm::Id index = 0; index < numElem; index++) {
                lines->el()[index] = 2 * index;
            }
            lines->el()[numElem] = numConn;
            result = lines;
        } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_POLY_LINE) {
            auto elements = isoGrid.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
            auto elemPortal = elements.ReadPortal();
            Lines::ptr lines(new Lines(numElem, numConn, numPoints));
            for (vtkm::Id index = 0; index < numConn; index++) {
                lines->cl()[index] = connPortal.Get(index);
            }
            for (vtkm::Id index = 0; index < numElem; index++) {
                lines->el()[index] = elemPortal.Get(index);
            }
            result = lines;
        } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE) {
            Triangles::ptr triangles(new Triangles(numConn, numPoints));
            for (vtkm::Id index = 0; index < numConn; index++) {
                triangles->cl()[index] = connPortal.Get(index);
            }
            result = triangles;
        } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_QUAD) {
            Quads::ptr quads(new Quads(numConn, numPoints));
            for (vtkm::Id index = 0; index < numConn; index++) {
                quads->cl()[index] = connPortal.Get(index);
            }
            result = quads;
        } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_POLYGON) {
            auto elements = isoGrid.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
            auto elemPortal = elements.ReadPortal();

            Polygons::ptr polys(new Polygons(numElem, numConn, numPoints));

            for (vtkm::Id index = 0; index < numConn; index++) {
                polys->cl()[index] = connPortal.Get(index);
            }
            for (vtkm::Id index = 0; index < numElem; index++) {
                polys->el()[index] = elemPortal.Get(index);
            }
            result = polys;
        }
    }

    if (!result && cellset.CanConvert<vtkm::cont::CellSetExplicit<>>()) {
        auto ecellset = cellset.AsCellSet<vtkm::cont::CellSetExplicit<>>();
        auto elements = ecellset.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
        auto elemPortal = elements.ReadPortal();
        auto numElem = ecellset.GetNumberOfCells();
        int mindim = 5, maxdim = -1;
        auto eshapes = ecellset.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
        auto eshapePortal = eshapes.ReadPortal();
        auto econn = ecellset.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
        auto econnPortal = econn.ReadPortal();
        auto numConn = econn.GetNumberOfValues();
        for (vtkm::Id idx = 0; idx < numElem; ++idx) {
            auto shape = eshapePortal.Get(idx);
            int dim = -1;
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

            mindim = std::min(dim, mindim);
            maxdim = std::max(dim, maxdim);
        }

        if (mindim > maxdim) {
            // empty
        } else if (mindim == -1) {
            // unhanded cell type
        } else if (mindim != maxdim || maxdim == 3) {
            // require UnstructuredGrid for mixed cells
            UnstructuredGrid::ptr unstr(new UnstructuredGrid(numElem, numConn, numPoints));
            for (vtkm::Id index = 0; index < numConn; index++) {
                unstr->cl()[index] = econnPortal.Get(index);
            }
            for (vtkm::Id index = 0; index < numElem + 1; index++) {
                unstr->el()[index] = elemPortal.Get(index);
            }
            for (vtkm::Id index = 0; index < numElem; index++) {
                unstr->tl()[index] = eshapePortal.Get(index);
            }
            result = unstr;
        } else if (mindim == 0) {
            Points::ptr points(new Points(numPoints));
            result = points;
        } else if (mindim == 1) {
            Lines::ptr lines(new Lines(numElem, numConn, numPoints));
            for (vtkm::Id index = 0; index < numConn; index++) {
                lines->cl()[index] = econnPortal.Get(index);
            }
            for (vtkm::Id index = 0; index < numElem + 1; index++) {
                lines->el()[index] = elemPortal.Get(index);
            }
            result = lines;
        } else if (mindim == 2) {
            // all 2D cells representable as Polygons
            Polygons::ptr polys(new Polygons(numElem, numConn, numPoints));
            for (vtkm::Id index = 0; index < numConn; index++) {
                polys->cl()[index] = econnPortal.Get(index);
            }
            for (vtkm::Id index = 0; index < numElem + 1; index++) {
                polys->el()[index] = elemPortal.Get(index);
            }
            result = polys;
        }
    }

    if (auto coords = Coords::as(result)) {
        auto x = coords->x().data();
        auto y = coords->y().data();
        auto z = coords->z().data();
        for (vtkm::Id index = 0; index < numPoints; index++) {
            vtkm::Vec3f point = pointsPortal.Get(index);
            // account for coordinate axes swap
            x[index] = point[2];
            y[index] = point[1];
            z[index] = point[0];
        }

        if (auto normals = vtkmGetField(dataset, "normals")) {
            if (auto nvec = vistle::Vec<vistle::Scalar, 3>::as(normals)) {
                auto n = std::make_shared<vistle::Normals>(0);
                n->d()->x[0] = nvec->d()->x[0];
                n->d()->x[1] = nvec->d()->x[1];
                n->d()->x[2] = nvec->d()->x[2];
                // don't use setNormals() in order to bypass check() on object before updateMeta()
                coords->d()->normals = n;
            } else {
                std::cerr << "cannot convert normals" << std::endl;
            }
        }
    }

    if (dataset.HasGhostCellField()) {
        auto ghostname = dataset.GetGhostCellFieldName();
        std::cerr << "vtkm: has ghost cells: " << ghostname << std::endl;
        if (auto ghosts = vtkmGetField(dataset, ghostname)) {
            std::cerr << "vtkm: got ghost cells: #" << ghosts->getSize() << std::endl;
            if (auto bvec = vistle::Vec<vistle::Byte>::as(ghosts)) {
                if (auto indexed = Indexed::as(result)) {
                    indexed->d()->ghost = bvec->d()->x[0];
                } else if (auto tri = Triangles::as(result)) {
                    tri->d()->ghost = bvec->d()->x[0];
                } else if (auto quad = Quads::as(result)) {
                    quad->d()->ghost = bvec->d()->x[0];
                } else {
                    std::cerr << "cannot apply ghosts" << std::endl;
                }
            } else {
                std::cerr << "cannot convert ghosts" << std::endl;
            }
        }
    }

    return result;
}


template<typename C>
struct Vistle;

#define MAP(vtkm_t, vistle_t) \
    template<> \
    struct Vistle<vtkm_t> { \
        typedef vistle_t type; \
    }

MAP(vtkm::Int8, vistle::Byte);
MAP(vtkm::UInt8, vistle::Byte);
MAP(vtkm::Int16, vistle::Index);
MAP(vtkm::UInt16, vistle::Index);
MAP(vtkm::Int32, vistle::Index);
MAP(vtkm::UInt32, vistle::Index);
MAP(vtkm::Int64, vistle::Index);
MAP(vtkm::UInt64, vistle::Index);
MAP(float, vistle::Scalar);
MAP(double, vistle::Scalar);

#undef MAP

struct GetArrayContents {
    vistle::DataBase::ptr &result;

    GetArrayContents(vistle::DataBase::ptr &result): result(result) {}

    template<typename T, typename S>
    VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S> &array) const
    {
        this->GetArrayPortal(array.ReadPortal());
    }

    template<typename PortalType>
    VTKM_CONT void GetArrayPortal(const PortalType &portal) const
    {
        using ValueType = typename PortalType::ValueType;
        using VTraits = vtkm::VecTraits<ValueType>;
        typedef typename Vistle<typename VTraits::ComponentType>::type V;
        ValueType dummy{};
        const vtkm::IdComponent numComponents = VTraits::GetNumberOfComponents(dummy);
        assert(!result);
        V *x[4] = {};
        switch (numComponents) {
        case 1: {
            auto data = std::make_shared<vistle::Vec<V, 1>>(portal.GetNumberOfValues());
            result = data;
            for (int i = 0; i < numComponents; ++i) {
                x[i] = &data->x(i)[0];
            }
            break;
        }
        case 2: {
            auto data = std::make_shared<vistle::Vec<V, 2>>(portal.GetNumberOfValues());
            result = data;
            for (int i = 0; i < numComponents; ++i) {
                x[i] = &data->x(i)[0];
            }
            break;
        }
        case 3: {
            auto data = std::make_shared<vistle::Vec<V, 3>>(portal.GetNumberOfValues());
            result = data;
            for (int i = 0; i < numComponents; ++i) {
                x[i] = &data->x(i)[0];
            }
            std::swap(x[0], x[2]);
            break;
        }
#if 0
        case 4: {
            auto data = std::make_shared<vistle::Vec<V, 4>>(portal.GetNumberOfValues());
            result = data;
            for (int i = 0; i < numComponents; ++i) {
                x[i] = &data->x(i)[0];
            }
            break;
        }
#endif
        }
        for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++) {
            ValueType value = portal.Get(index);
            for (vtkm::IdComponent componentIndex = 0; componentIndex < numComponents; componentIndex++) {
                x[componentIndex][index] = VTraits::GetComponent(value, componentIndex);
            }
        }
    }
};


vistle::DataBase::ptr vtkmGetField(const vtkm::cont::DataSet &vtkmDataSet, const std::string &name)
{
    vistle::DataBase::ptr result;
    if (!vtkmDataSet.HasField(name))
        return result;

    auto field = vtkmDataSet.GetField(name);
    if (!field.IsCellField() && !field.IsPointField())
        return result;
    auto ah = field.GetData();
    try {
        ah.CastAndCallForTypes<vtkm::TypeListAll, vtkm::cont::StorageListCommon>(GetArrayContents{result});
    } catch (vtkm::cont::ErrorBadType &err) {
        std::cerr << "cast error: " << err.what() << std::endl;
    }
    return result;
}

} // namespace vistle
