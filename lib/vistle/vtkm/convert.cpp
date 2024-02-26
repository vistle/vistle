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
#include <vistle/core/spheres.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/polygons.h>

#include <boost/mpl/for_each.hpp>

#include <stdexcept>

#include "convert.h"
#include "cellsetToVtkm.h"


// TODO: - better: make Vistle lines work, s.t., coords do not have to be same size as el
//       - doesn't the axes swap affect the normals, too?

// BUG: - (IsoSurface, PointPerTimestep, isopoint): when "moving" the grid (changing x in Gendat),
//         isopoints still work outside the new bounding box (where old bounding box was)

namespace vistle {

VtkmTransformStatus coordinatesToVtkm(vistle::Object::const_ptr grid, vtkm::cont::DataSet &vtkmDataset)
{
    if (auto coordinates = Coords::as(grid)) {
        auto coordinateSystem = vtkm::cont::CoordinateSystem(
            "coordinate system",
            vtkm::cont::make_ArrayHandleSOA(coordinates->x(xId).handle(), coordinates->x(yId).handle(),
                                            coordinates->x(zId).handle()));

        vtkmDataset.AddCoordinateSystem(coordinateSystem);

        if (coordinates->normals())
            vtkmAddField(vtkmDataset, coordinates->normals(), "normals");

    } else if (auto uni = UniformGrid::as(grid)) {
        auto axesDivisions = vtkm::Id3(uni->getNumDivisions(xId), uni->getNumDivisions(yId), uni->getNumDivisions(zId));
        auto gridOrigin = vtkm::Vec3f{uni->min()[xId], uni->min()[yId], uni->min()[zId]};
        auto cellSizes = vtkm::Vec3f{uni->dist()[xId], uni->dist()[yId], uni->dist()[zId]};

        vtkm::cont::ArrayHandleUniformPointCoordinates implicitCoordinates(axesDivisions, gridOrigin, cellSizes);

        vtkmDataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("uniform", implicitCoordinates));

    } else if (auto rect = RectilinearGrid::as(grid)) {
        vtkm::cont::ArrayHandleCartesianProduct implicitCoordinates(
            rect->coords(xId).handle(), rect->coords(yId).handle(), rect->coords(zId).handle());
        vtkmDataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("rectilinear", implicitCoordinates));

    } else {
        return VtkmTransformStatus::UNSUPPORTED_GRID_TYPE;
    }

    return VtkmTransformStatus::SUCCESS;
}

void ghostToVtkm(vistle::Object::const_ptr grid, vtkm::cont::DataSet &vtkmDataset)
{
    vtkmDataset.SetGhostCellFieldName("ghost");

    if (auto indexedGrid = Indexed::as(grid)) {
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
}

VtkmTransformStatus gridToVtkm(vistle::Object::const_ptr grid, vtkm::cont::DataSet &vtkmDataset)
{
    auto status = coordinatesToVtkm(grid, vtkmDataset);
    if (status != VtkmTransformStatus::SUCCESS)
        return status;

    status = cellsetToVtkm(grid, vtkmDataset);
    if (status != VtkmTransformStatus::SUCCESS)
        return status;

    ghostToVtkm(grid, vtkmDataset);

    return VtkmTransformStatus::SUCCESS;
}

// CONVERT DATA FIELD (VISTLE TO VTKM)
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
            auto ax = in->x(xId).handle();
            auto ay = in->x(yId).handle();
            auto az = in->x(zId).handle();
            ah = vtkm::cont::make_ArrayHandleSOA(ax, ay, az);
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

Object::ptr cellSetSingleTypeToVistle(vtkm::cont::DataSet &dataset, vtkm::Id numPoints)
{
    Object::ptr result;

    auto cellset = dataset.GetCellSet();

    // try conversion for uniform cell types first
    auto isoGrid = cellset.AsCellSet<vtkm::cont::CellSetSingleType<>>();
    // get connectivity array of the dataset
    auto connectivity = isoGrid.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    auto connPortal = connectivity.ReadPortal();
    auto numConn = connectivity.GetNumberOfValues();
    auto numElem = cellset.GetNumberOfCells();

    // 1a1) VERTEX --> Points::ptr
    if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_VERTEX) {
        Points::ptr points(new Points(numPoints));
        result = points;

        // 1a2) LINE --> Lines::ptr
    } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_LINE) {
        auto numElem = numConn > 0 ? numConn / 2 : numPoints / 2;
        //Lines::ptr lines(new Lines(numElem, numConn, numPoints));
        Lines::ptr lines(new Lines(numElem, numConn, numConn));
        for (vtkm::Id index = 0; index < numConn; index++) {
            lines->cl()[index] = connPortal.Get(index);
        }
        // TODO: this belongs in 2)
        if (dataset.GetNumberOfCoordinateSystems() > 0) {
            auto vtkmCoords = dataset.GetCoordinateSystem().GetData();

            if (vtkmCoords.CanConvert<vtkm::cont::ArrayHandle<vtkm::Vec3f>>()) {
                auto coordsPortal = vtkmCoords.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>().ReadPortal();
                for (vtkm::Id index = 0; index < numConn; index++) {
                    auto point = coordsPortal.Get(connPortal.Get(index));
                    (lines->x().data())[index] = point[xId];
                    (lines->y().data())[index] = point[yId];
                    (lines->z().data())[index] = point[zId];
                }
            } else if (vtkmCoords.CanConvert<vtkm::cont::ArrayHandleSOA<vtkm::Vec3f>>()) {
                auto coordsPortal = vtkmCoords.AsArrayHandle<vtkm::cont::ArrayHandleSOA<vtkm::Vec3f>>().ReadPortal();
                for (vtkm::Id index = 0; index < numConn; index++) {
                    auto point = coordsPortal.Get(connPortal.Get(index));
                    (lines->x().data())[index] = point[xId];
                    (lines->y().data())[index] = point[yId];
                    (lines->z().data())[index] = point[zId];
                }
            } else {
                throw std::invalid_argument("VTKm coordinate system uses unsupported array handle storage.");
            }
        }

        // TODO: check if this breaks anything
        for (vtkm::Id index = 0; index < numElem; index++) {
            lines->el()[index] = 2 * index;
            lines->cl()[2 * index] = 2 * index;
            lines->cl()[2 * index + 1] = 2 * index + 1;
        }
        lines->el()[numElem] = numConn;
        result = lines;

        // 1a3) POLY_LINE --> Lines::ptr
    } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_POLY_LINE) {
        auto elements = isoGrid.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
        auto elemPortal = elements.ReadPortal();
        Lines::ptr lines(new Lines(numElem, numConn, numPoints));
        for (vtkm::Id index = 0; index < numConn; index++) {
            lines->cl()[index] = connPortal.Get(index);
        }
        for (vtkm::Id index = 0; index < numElem + 1; index++) {
            lines->el()[index] = elemPortal.Get(index);
        }
        result = lines;

        // 1a4) TRIANGLE --> Triangles::ptr
    } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE) {
        Triangles::ptr triangles(new Triangles(numConn, numPoints));
        for (vtkm::Id index = 0; index < numConn; index++) {
            triangles->cl()[index] = connPortal.Get(index);
        }
        result = triangles;

        // 1a5) QUAD --> Quads::ptr
    } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_QUAD) {
        Quads::ptr quads(new Quads(numConn, numPoints));
        for (vtkm::Id index = 0; index < numConn; index++) {
            quads->cl()[index] = connPortal.Get(index);
        }
        result = quads;

        // 1a6) POLYGON --> Polygons::ptr
    } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_POLYGON) {
        auto elements = isoGrid.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
        auto elemPortal = elements.ReadPortal();

        Polygons::ptr polys(new Polygons(numElem, numConn, numPoints));

        for (vtkm::Id index = 0; index < numConn; index++) {
            polys->cl()[index] = connPortal.Get(index);
        }
        for (vtkm::Id index = 0; index < numElem + 1; index++) {
            polys->el()[index] = elemPortal.Get(index);
        }
        result = polys;
    }
    return result;
}

Object::ptr cellSetExplicitToVistle(vtkm::cont::DataSet &dataset, vtkm::Id numPoints)
{
    Object::ptr result;

    auto cellset = dataset.GetCellSet();

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
    return result;
}

// ----- VTKm to VISTLE -----
Object::ptr cellsetToVistle(vtkm::cont::DataSet &dataset)
{
    auto cellset = dataset.GetCellSet();

    Object::ptr result;

    auto numPoints = dataset.GetNumberOfPoints();
    if (dataset.GetNumberOfCoordinateSystems() > 0) // TODO: check if this breaks anything
        numPoints = dataset.GetCoordinateSystem().GetNumberOfPoints();

    // try conversion for uniform cell types first
    if (cellset.CanConvert<vtkm::cont::CellSetSingleType<>>()) {
        result = cellSetSingleTypeToVistle(dataset, numPoints);
    }

    // 1b) Convert CellSetExplicit: cells can be of different types
    if (!result && cellset.CanConvert<vtkm::cont::CellSetExplicit<>>()) {
        result = cellSetExplicitToVistle(dataset, numPoints);
    }

    return result;
}

template<typename ArrayHandlePortal>
void fillVistleCoords(ArrayHandlePortal coordsPortal, Coords::ptr coords)
{
    auto x = coords->x().data();
    auto y = coords->y().data();
    auto z = coords->z().data();

    for (vtkm::Id index = 0; index < coordsPortal.GetNumberOfValues(); index++) {
        vtkm::Vec3f point = coordsPortal.Get(index);
        x[index] = point[xId];
        y[index] = point[yId];
        z[index] = point[zId];
    }
}

void vtkmToVistleCoordinateSystem(const vtkm::cont::CoordinateSystem &coordinateSystem, Coords::ptr coords)
{
    auto vtkmCoords = coordinateSystem.GetData();

    if (vtkmCoords.CanConvert<vtkm::cont::ArrayHandle<vtkm::Vec3f>>()) {
        auto coordsPortal = vtkmCoords.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>().ReadPortal();
        fillVistleCoords(coordsPortal, coords);
    } else if (vtkmCoords.CanConvert<vtkm::cont::ArrayHandleSOA<vtkm::Vec3f>>()) {
        auto coordsPortal = vtkmCoords.AsArrayHandle<vtkm::cont::ArrayHandleSOA<vtkm::Vec3f>>().ReadPortal();
        fillVistleCoords(coordsPortal, coords);
    } else {
        throw std::invalid_argument("VTKm coordinate system uses unsupported array handle storage.");
    }
}

void addVtkmCoordinatesToVistle(vtkm::cont::DataSet &dataset, Object::ptr result)
{
    if (auto coords = Coords::as(result)) {
        if (!Lines::as(result) && dataset.GetNumberOfCoordinateSystems() > 0)
            vtkmToVistleCoordinateSystem(dataset.GetCoordinateSystem(), coords);

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
}

void addVtkmGhostInfoToVistle(vtkm::cont::DataSet &dataset, Object::ptr result)
{
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
}

Object::ptr vtkmGetGeometry(vtkm::cont::DataSet &dataset)
{
    auto result = cellsetToVistle(dataset);

    addVtkmCoordinatesToVistle(dataset, result);

    addVtkmGhostInfoToVistle(dataset, result);

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

// CONVERT DATA FIELD (VTKm to VISTLE)
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
