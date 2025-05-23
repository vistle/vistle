#include "convert.h"
#include "convert_topology.h"

#include <viskores/cont/ArrayHandleExtractComponent.h>

#include <vistle/core/scalars.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/points.h>
#include <vistle/core/lines.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/indexed.h>

#include <vistle/core/shm_array_impl.h>
#include <vistle/core/shm_obj_ref_impl.h>

#include <boost/mpl/for_each.hpp>


namespace vistle {

namespace {

ModuleStatusPtr vtkmApplyGhost(viskores::cont::DataSet &vtkmDataset, Object::const_ptr &grid)
{
    const std::string &ghostname = "ghost";
    vtkmDataset.SetGhostCellFieldName(ghostname);

    if (auto indexedGrid = Indexed::as(grid)) {
        if (indexedGrid->ghost().size() > 0) {
            auto ghost = indexedGrid->ghost().handle();
            vtkmDataset.SetGhostCellField(ghostname, ghost);
        }
    } else if (auto tri = Triangles::as(grid)) {
        if (tri->ghost().size() > 0) {
            auto ghost = tri->ghost().handle();
            vtkmDataset.SetGhostCellField(ghostname, ghost);
        }
    } else if (auto quad = Quads::as(grid)) {
        if (quad->ghost().size() > 0) {
            auto ghost = quad->ghost().handle();
            vtkmDataset.SetGhostCellField(ghostname, ghost);
        }
    }
    return Success();
}

ModuleStatusPtr vtkmGetGhosts(const viskores::cont::DataSet &dataset, Object::ptr &result)
{
    if (!dataset.HasGhostCellField())
        return Success();

    auto ghostname = dataset.GetGhostCellFieldName();
    std::cerr << "vtkm: has ghost cells: " << ghostname << std::endl;
    if (auto ghosts = vtkmGetField(dataset, ghostname, DataBase::Element)) {
        std::cerr << "vtkm: got ghost cells: #" << ghosts->getSize() << std::endl;
        if (auto bvec = vistle::Vec<vistle::Byte>::as(ghosts)) {
            if (auto indexed = Indexed::as(result)) {
                indexed->d()->ghost = bvec->d()->x[0];
                return Success();
            } else if (auto tri = Triangles::as(result)) {
                tri->d()->ghost = bvec->d()->x[0];
                return Success();
            } else if (auto quad = Quads::as(result)) {
                quad->d()->ghost = bvec->d()->x[0];
                return Success();
            }

            return Error("Cannot apply VTKm ghost field to Vistle grid: only supported for Indexed, Triangles, and "
                         "Quads grids.");
        }
        return Error("The converted ghost field is not of type Byte.");
    }
    return Error(
        "An error occurred while converting VTKm ghost field to Vistle grid. Check logfile for more information.");
}

ModuleStatusPtr vtkmApplyRadius(viskores::cont::DataSet &vtkmDataset, Object::const_ptr &grid)
{
    vistle::Vec<Scalar>::const_ptr radius;
    if (auto lines = Lines::as(grid)) {
        radius = lines->radius();
    } else if (auto points = Points::as(grid)) {
        radius = points->radius();
    }
    if (radius) {
        return vtkmAddField(vtkmDataset, radius, "_radius");
    }
    return Success();
}

ModuleStatusPtr vtkmGetRadius(const viskores::cont::DataSet &dataset, Object::ptr &result)
{
    auto radius = vtkmGetField(dataset, "_radius");
    if (!radius) {
        return Success();
    }
    auto rvec = vistle::Vec<vistle::Scalar>::as(radius);
    if (!rvec) {
        return Error("Radius must be a scalar field.");
    }

    auto r = std::make_shared<vistle::Vec<Scalar>>(0);
    r->d()->x[0] = rvec->d()->x[0];
    // don't use setRadius() in order to bypass check() on object before updateMeta()
    if (auto lines = Lines::as(result)) {
        lines->d()->radius = r;
        return Success();
    } else if (auto points = Points::as(result)) {
        points->d()->radius = r;
        return Success();
    }

    return Error("Radius can only be applied to Points and Lines.");
}

ModuleStatusPtr vtkmApplyNormals(viskores::cont::DataSet &vtkmDataset, Object::const_ptr &grid)
{
    auto coords = Coords::as(grid);
    if (!coords) {
        return Success();
    }

    auto normals = coords->normals();
    if (!normals) {
        return Success();
    }

    auto mapping = normals->guessMapping(coords);
    return vtkmAddField(vtkmDataset, normals, "normals", mapping);
}

ModuleStatusPtr vtkmGetNormals(const viskores::cont::DataSet &dataset, Object::ptr &result)
{
    auto normals = vtkmGetField(dataset, "normals");
    if (!normals) {
        normals = vtkmGetField(dataset, "Normals");
    }
    if (!normals) {
        return Success();
    }
    auto nvec = vistle::Vec<vistle::Scalar, 3>::as(normals);
    if (!nvec) {
        return Error("Normals must be a 3D vector field.");
    }
    auto coords = Coords::as(result);
    if (!coords) {
        return Error("Normals can only be applied to grids with coordinates.");
    }

    auto n = std::make_shared<vistle::Normals>(0);
    n->d()->x[0] = nvec->d()->x[0];
    n->d()->x[1] = nvec->d()->x[1];
    n->d()->x[2] = nvec->d()->x[2];
    // don't use setNormals() in order to bypass check() on object before updateMeta()
    coords->d()->normals = n;

    return Success();
}

} // namespace

ModuleStatusPtr vtkmSetGrid(viskores::cont::DataSet &vtkmDataset, vistle::Object::const_ptr grid)
{
    if (auto coords = Coords::as(grid)) {
        auto coordinateSystem = viskores::cont::CoordinateSystem(
            "coordinate system",
            viskores::cont::make_ArrayHandleSOA(coords->x().handle(), coords->y().handle(), coords->z().handle()));

        vtkmDataset.AddCoordinateSystem(coordinateSystem);

        vtkmApplyNormals(vtkmDataset, grid);
        vtkmApplyRadius(vtkmDataset, grid);
    } else if (auto uni = UniformGrid::as(grid)) {
        auto nx = uni->getNumDivisions(0);
        auto ny = uni->getNumDivisions(1);
        auto nz = uni->getNumDivisions(2);
        const auto *min = uni->min(), *dist = uni->dist();
        viskores::cont::ArrayHandleUniformPointCoordinates uniformCoordinates(
            viskores::Id3(nx, ny, nz), viskores::Vec3f{min[0], min[1], min[2]},
            viskores::Vec3f{dist[0], dist[1], dist[2]});
        auto coordinateSystem = viskores::cont::CoordinateSystem("uniform", uniformCoordinates);
        vtkmDataset.AddCoordinateSystem(coordinateSystem);
    } else if (auto rect = RectilinearGrid::as(grid)) {
        auto xc = rect->coords(0).handle();
        auto yc = rect->coords(1).handle();
        auto zc = rect->coords(2).handle();

        viskores::cont::ArrayHandleCartesianProduct rectilinearCoordinates(xc, yc, zc);
        auto coordinateSystem = viskores::cont::CoordinateSystem("rectilinear", rectilinearCoordinates);
        vtkmDataset.AddCoordinateSystem(coordinateSystem);
    } else {
        return Error("Found unsupported grid type while attempting to convert Vistle grid to Viskores dataset.");
    }

    auto stat = vtkmSetTopology(vtkmDataset, grid);
    if (!stat->continueExecution()) {
        return stat;
    }
    return vtkmApplyGhost(vtkmDataset, grid);
}

namespace {
struct AddField {
    viskores::cont::DataSet &dataset;
    const DataBase::const_ptr &object;
    DataBase::Mapping mapping;
    const std::string &name;
    bool &handled;
    AddField(viskores::cont::DataSet &ds, const DataBase::const_ptr &obj, const std::string &name,
             DataBase::Mapping mapping, bool &handled)
    : dataset(ds), object(obj), mapping(mapping), name(name), handled(handled)
    {}
    template<typename S>
    void operator()(S)
    {
        typedef Vec<S, 1> V1;
        //typedef Vec<S, 2> V2;
        typedef Vec<S, 3> V3;
        //typedef Vec<S, 4> V4;

        viskores::cont::UnknownArrayHandle ah;
        if (auto in = V1::as(object)) {
            ah = in->x().handle();
        } else if (auto in = V3::as(object)) {
            auto ax = in->x().handle();
            auto ay = in->y().handle();
            auto az = in->z().handle();
            ah = viskores::cont::make_ArrayHandleSOA(ax, ay, az);
        } else {
            return;
        }

        auto mapping = this->mapping;
        if (mapping == DataBase::Unspecified) {
            mapping = object->guessMapping();
        }
        if (mapping == vistle::DataBase::Vertex) {
            dataset.AddPointField(name, ah);
        } else {
            dataset.AddCellField(name, ah);
        }

        handled = true;
    }
};
} // namespace

ModuleStatusPtr vtkmAddField(viskores::cont::DataSet &vtkmDataSet, const vistle::DataBase::const_ptr &field,
                             const std::string &name, vistle::DataBase::Mapping mapping)
{
    bool handled = false;
    boost::mpl::for_each<Scalars>(AddField(vtkmDataSet, field, name, mapping, handled));
    if (handled)
        return Success();

    return Error("Encountered an unsupported field type while attempting to convert Vistle field to Viskores field.");
}


Object::ptr vtkmGetGeometry(const viskores::cont::DataSet &dataset)
{
    Object::ptr result = vtkmGetTopology(dataset);

    if (auto coords = Coords::as(result)) {
        auto uPointCoordinates = dataset.GetCoordinateSystem().GetData();
        viskores::cont::UnknownArrayHandle unknown(uPointCoordinates);
        if (unknown.CanConvert<viskores::cont::ArrayHandle<viskores::Vec<Scalar, 3>>>()) {
            auto vtkmCoord = unknown.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec<Scalar, 3>>>();
            for (int d = 0; d < 3; ++d) {
                auto x = make_ArrayHandleExtractComponent(vtkmCoord, d);
                coords->d()->x[d]->setHandle(x);
            }
        } else if (unknown.CanConvert<viskores::cont::ArrayHandleSOA<viskores::Vec3f>>()) {
            auto vtkmCoord = unknown.AsArrayHandle<viskores::cont::ArrayHandleSOA<viskores::Vec3f>>();
            for (int d = 0; d < 3; ++d) {
                auto x = vtkmCoord.GetArray(d);
                coords->d()->x[d]->setHandle(x);
            }
        } else {
            std::cerr << "cannot convert point coordinates" << std::endl;
        }

        vtkmGetNormals(dataset, result);
        vtkmGetRadius(dataset, result);
    }

    vtkmGetGhosts(dataset, result);
    return result;
}


namespace {
template<typename C>
struct Vistle;

#define MAP(vtkm_t, vistle_t) \
    template<> \
    struct Vistle<vtkm_t> { \
        typedef vistle_t type; \
    }

MAP(viskores::Int8, vistle::Byte);
MAP(viskores::UInt8, vistle::Byte);
MAP(viskores::Int16, vistle::Index);
MAP(viskores::UInt16, vistle::Index);
MAP(viskores::Int32, vistle::Index);
MAP(viskores::UInt32, vistle::Index);
MAP(viskores::Int64, vistle::Index);
MAP(viskores::UInt64, vistle::Index);
MAP(float, vistle::Scalar);
MAP(double, vistle::Scalar);

#undef MAP

struct GetArrayContents {
    vistle::DataBase::ptr &result;

    GetArrayContents(vistle::DataBase::ptr &result): result(result) {}

    template<typename V, viskores::IdComponent Dim, typename T, typename S>
    typename vistle::Vec<V, Dim>::ptr makeVec(const viskores::cont::ArrayHandle<T, S> &array) const
    {
        auto data = std::make_shared<vistle::Vec<V, Dim>>(array.GetNumberOfValues());
        for (int i = 0; i < Dim; ++i) {
            auto comp = viskores::cont::make_ArrayHandleExtractComponent(array, i);
            data->d()->x[i]->setHandle(comp);
        }
        return data;
    }

    template<typename T, typename S>
    VISKORES_CONT void operator()(const viskores::cont::ArrayHandle<T, S> &array) const
    {
        using ValueType = T;
        using VTraits = viskores::VecTraits<ValueType>;
        typedef typename Vistle<typename VTraits::ComponentType>::type V;
        ValueType dummy{};
        const viskores::IdComponent numComponents = VTraits::GetNumberOfComponents(dummy);
        assert(!result);
        switch (numComponents) {
        case 1: {
            result = makeVec<V, 1>(array);
            break;
        }
        case 2: {
            result = makeVec<V, 2>(array);
            break;
        }
        case 3: {
            result = makeVec<V, 3>(array);
            break;
        }
#if 0
        case 4: {
            result = makeVec<V, 4>(array);
            break;
        }
#endif
        }
    }
};
} // namespace

vistle::DataBase::ptr vtkmGetField(const viskores::cont::DataSet &vtkmDataSet, const std::string &name,
                                   vistle::DataBase::Mapping mapping)
{
    vistle::DataBase::ptr result;
    if (!vtkmDataSet.HasField(name))
        return result;

    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any;
    switch (mapping) {
    case vistle::DataBase::Vertex:
        assoc = viskores::cont::Field::Association::Points;
        break;
    case vistle::DataBase::Element:
        assoc = viskores::cont::Field::Association::Cells;
        break;
    case vistle::DataBase::Unspecified:
        // leave it at Any
        break;
    default:
        assert("Invalid mapping type" == nullptr);
        break;
    }

    auto field = vtkmDataSet.GetField(name, assoc);
    if (!field.IsCellField() && !field.IsPointField()) {
        std::cerr << "Viskores field " << name << " is neither point nor cell field" << std::endl;
        return result;
    }
    auto ah = field.GetData();
    try {
        ah.CastAndCallForTypes<viskores::TypeListAll, viskores::cont::StorageListCommon>(GetArrayContents{result});
    } catch (viskores::cont::ErrorBadType &err) {
        std::cerr << "cast error: " << err.what() << std::endl;
    }
    if (result) {
        if (field.IsCellField()) {
            result->setMapping(vistle::DataBase::Element);
        } else {
            result->setMapping(vistle::DataBase::Vertex);
        }
    }
    return result;
}

} // namespace vistle
