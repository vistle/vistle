#include <boost/mpl/for_each.hpp>
#include <stdexcept>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandleExtractComponent.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/scalars.h>
#include <vistle/core/shm_array_impl.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/triangles.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/unstr.h>

#include "convert.h"
#include "convert_worklets.h"
#include "cellsetToVtkm.h"

namespace vistle {

// ----- VISTLE to VTKm -----
VtkmTransformStatus coordinatesAndNormalsToVtkm(Object::const_ptr grid, vtkm::cont::DataSet &vtkmDataset)
{
    if (auto coordinates = Coords::as(grid)) {
        auto coordinateSystem = vtkm::cont::CoordinateSystem(
            "coordinate system",
            vtkm::cont::make_ArrayHandleSOA(coordinates->x(xId).handle(), coordinates->x(yId).handle(),
                                            coordinates->x(zId).handle()));

        vtkmDataset.AddCoordinateSystem(coordinateSystem);

        if (coordinates->normals()) {
            auto normals = coordinates->normals();
            auto mapping = normals->guessMapping(coordinates);
            fieldToVtkm(normals, vtkmDataset, "normals", mapping);
        }

        vistle::Vec<Scalar>::const_ptr radius;
        if (auto lines = Lines::as(grid)) {
            radius = lines->radius();
        } else if (auto points = Points::as(grid)) {
            radius = points->radius();
        }
        if (radius) {
            fieldToVtkm(radius, vtkmDataset, "_radius");
        }

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

void ghostToVtkm(Object::const_ptr grid, vtkm::cont::DataSet &vtkmDataset)
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

VtkmTransformStatus geometryToVtkm(Object::const_ptr grid, vtkm::cont::DataSet &vtkmDataset)
{
    auto status = coordinatesAndNormalsToVtkm(grid, vtkmDataset);
    if (status != VtkmTransformStatus::SUCCESS)
        return status;

    status = cellsetToVtkm(grid, vtkmDataset);
    if (status != VtkmTransformStatus::SUCCESS)
        return status;

    ghostToVtkm(grid, vtkmDataset);

    return VtkmTransformStatus::SUCCESS;
}

namespace {
struct FieldToVtkm {
    const DataBase::const_ptr &m_field;
    vtkm::cont::DataSet &m_dataset;
    const std::string &m_name;
    DataBase::Mapping m_mapping;
    VtkmTransformStatus &m_status;

    FieldToVtkm(const DataBase::const_ptr &field, vtkm::cont::DataSet &dataset, const std::string &name,
                DataBase::Mapping mapping, VtkmTransformStatus &status)
    : m_field(field), m_dataset(dataset), m_name(name), m_mapping(mapping), m_status(status)
    {}

    template<typename ScalarType>
    void operator()(ScalarType)
    {
        typedef Vec<ScalarType, 1> V1;
        typedef Vec<ScalarType, 3> V3;

        vtkm::cont::UnknownArrayHandle ah;
        if (auto in = V1::as(m_field)) {
            ah = in->x().handle();

        } else if (auto in = V3::as(m_field)) {
            ah = vtkm::cont::make_ArrayHandleSOA(in->x(xId).handle(), in->x(yId).handle(), in->x(zId).handle());

        } else {
            return;
        }

        if (m_mapping == DataBase::Unspecified) {
            m_mapping = m_field->guessMapping();
        }
        if (m_mapping == vistle::DataBase::Vertex) {
            m_dataset.AddPointField(m_name, ah);
        } else {
            m_dataset.AddCellField(m_name, ah);
        }

        m_status = VtkmTransformStatus::SUCCESS;
    }
};
} // namespace

VtkmTransformStatus fieldToVtkm(const DataBase::const_ptr &field, vtkm::cont::DataSet &vtkmDataset,
                                const std::string &fieldName, DataBase::Mapping mapping)
{
    VtkmTransformStatus status = VtkmTransformStatus::UNSUPPORTED_FIELD_TYPE;
    boost::mpl::for_each<Scalars>(FieldToVtkm(field, vtkmDataset, fieldName, mapping, status));

    return status;
}

// ------------ VTKM TO VISTLE -------------
void ghostToVistle(vtkm::cont::DataSet &dataset, Object::ptr result)
{
    if (dataset.HasGhostCellField()) {
        auto ghostname = dataset.GetGhostCellFieldName();
        std::cerr << "vtkm: has ghost cells: " << ghostname << std::endl;
        if (auto ghosts = vtkmFieldToVistle(dataset, ghostname)) {
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

void coordinatesAndNormalsToVistle(vtkm::cont::DataSet &dataset, Object::ptr result)
{
    if (auto coords = Coords::as(result)) {
        if (dataset.GetNumberOfCoordinateSystems() > 0) {
            auto unknown = dataset.GetCoordinateSystem().GetData();
            if (unknown.CanConvert<vtkm::cont::ArrayHandle<vtkm::Vec<Scalar, 3>>>()) {
                auto vtkmCoord = unknown.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec<Scalar, 3>>>();
                for (int d = 0; d < 3; ++d) {
                    auto x = make_ArrayHandleExtractComponent(vtkmCoord, d);
                    coords->d()->x[d]->setHandle(x);
                }
            } else if (unknown.CanConvert<vtkm::cont::ArrayHandleSOA<vtkm::Vec3f>>()) {
                auto vtkmCoord = unknown.AsArrayHandle<vtkm::cont::ArrayHandleSOA<vtkm::Vec3f>>();
                for (int d = 0; d < 3; ++d) {
                    auto x = vtkmCoord.GetArray(d);
                    coords->d()->x[d]->setHandle(x);
                }

            } else {
                std::cerr << "cannot convert point coordinates" << std::endl;
            }
        }

        if (auto normals = vtkmFieldToVistle(dataset, "normals")) {
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

        if (auto radius = vtkmFieldToVistle(dataset, "_radius")) {
            if (auto rvec = vistle::Vec<vistle::Scalar>::as(radius)) {
                auto r = std::make_shared<vistle::Vec<Scalar>>(0);
                r->d()->x[0] = rvec->d()->x[0];
                // don't use setRadius() in order to bypass check() on object before updateMeta()
                if (auto lines = Lines::as(result)) {
                    lines->d()->radius = r;
                }
                if (auto points = Points::as(result)) {
                    points->d()->radius = r;
                }
            } else {
                std::cerr << "cannot apply radius to anything but Points and Lines" << std::endl;
            }
        }
    }
}

Object::ptr cellSetSingleTypeToVistle(const vtkm::cont::DataSet &dataset, vtkm::Id numPoints)
{
    auto cellset = dataset.GetCellSet().AsCellSet<vtkm::cont::CellSetSingleType<>>();

    // get connectivity array of the dataset
    auto connectivity = cellset.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    auto numConn = connectivity.GetNumberOfValues();

    if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_VERTEX) {
        Points::ptr points(new Points(Object::Initialized));
        return points;
    } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_LINE) {
        auto numElem = numConn > 0 ? numConn / 2 : numPoints / 2;
        Lines::ptr lines(new Lines(numElem, 0, 0));
        lines->d()->cl->setHandle(connectivity);
        for (vtkm::Id index = 0; index < numElem; index++) {
            lines->el()[index] = 2 * index;
        }
        lines->el()[numElem] = numConn;
        return lines;
    } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_POLY_LINE) {
        auto elements = cellset.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
        Lines::ptr lines(new Lines(0, 0, 0));
        lines->d()->cl->setHandle(connectivity);
        lines->d()->el->setHandle(elements);
        return lines;
    } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE) {
        Triangles::ptr triangles(new Triangles(0, 0));
        triangles->d()->cl->setHandle(connectivity);
        return triangles;
    } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_QUAD) {
        Quads::ptr quads(new Quads(0, 0));
        quads->d()->cl->setHandle(connectivity);
        return quads;
    } else if (cellset.GetCellShape(0) == vtkm::CELL_SHAPE_POLYGON) {
        auto elements = cellset.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
        Polygons::ptr polys(new Polygons(0, 0, 0));
        polys->d()->cl->setHandle(connectivity);
        polys->d()->el->setHandle(elements);
        return polys;
    }
}

Object::ptr cellsetExplicitToVistle(const vtkm::cont::DataSet &dataset, vtkm::Id numPoints)
{
    auto cellset = dataset.GetCellSet().AsCellSet<vtkm::cont::CellSetExplicit<>>();
    auto elements = cellset.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    auto shapes = cellset.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    auto conn = cellset.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());

    const auto [mindim, maxdim] = getMinMaxDims(shapes);

    if (mindim != maxdim || maxdim == 3) {
        // require UnstructuredGrid for mixed cells
        UnstructuredGrid::ptr unstr(new UnstructuredGrid(0, 0, 0));
        unstr->d()->cl->setHandle(conn);
        unstr->d()->el->setHandle(elements);
        unstr->d()->tl->setHandle(shapes);
        return unstr;
    } else if (mindim == 0) {
        Points::ptr points(new Points(Object::Initialized));
        return points;
    } else if (mindim == 1) {
        Lines::ptr lines(new Lines(0, 0, 0));
        lines->d()->cl->setHandle(conn);
        lines->d()->el->setHandle(elements);
        return lines;
    } else if (mindim == 2) {
        // all 2D cells representable as Polygons
        Polygons::ptr polys(new Polygons(0, 0, 0));
        polys->d()->cl->setHandle(conn);
        polys->d()->el->setHandle(elements);
        return polys;
    }
}

Object::ptr cellsetToVistle(const vtkm::cont::DataSet &dataset)
{
    auto cellset = dataset.GetCellSet();

    Object::ptr result;

    auto numPoints = dataset.GetNumberOfPoints();
    if (dataset.GetNumberOfCoordinateSystems() > 0)
        numPoints = dataset.GetCoordinateSystem().GetNumberOfPoints();

    // try conversion for uniform cell types first
    if (cellset.CanConvert<vtkm::cont::CellSetSingleType<>>()) {
        result = cellSetSingleTypeToVistle(dataset, numPoints);
    }

    if (!result && cellset.CanConvert<vtkm::cont::CellSetExplicit<>>()) {
        result = cellsetExplicitToVistle(dataset, numPoints);
    }
    return result;
}

Object::ptr vtkmGeometryToVistle(vtkm::cont::DataSet &dataset)
{
    auto result = cellsetToVistle(dataset);

    coordinatesAndNormalsToVistle(dataset, result);

    ghostToVistle(dataset, result);

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
    DataBase::ptr &result;

    GetArrayContents(DataBase::ptr &result): result(result) {}

    template<typename T, typename S>
    VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S> &array) const
    {
        using ValueType = T;
        using VTraits = vtkm::VecTraits<ValueType>;
        typedef typename Vistle<typename VTraits::ComponentType>::type V;
        ValueType dummy{};
        const vtkm::IdComponent numComponents = VTraits::GetNumberOfComponents(dummy);
        assert(!result);
        switch (numComponents) {
        case 1: {
            auto data = std::make_shared<vistle::Vec<V, 1>>(array.GetNumberOfValues());
            result = data;
            for (int i = 0; i < numComponents; ++i) {
                data->d()->x[i]->setHandle(array);
            }
            break;
        }
        case 2: {
            auto data = std::make_shared<vistle::Vec<V, 2>>(array.GetNumberOfValues());
            result = data;
            for (int i = 0; i < numComponents; ++i) {
                auto x = make_ArrayHandleExtractComponent(array, i);
                data->d()->x[i]->setHandle(x);
            }
            break;
        }
        case 3: {
            auto data = std::make_shared<vistle::Vec<V, 3>>(array.GetNumberOfValues());
            result = data;
            for (int i = 0; i < numComponents; ++i) {
                auto x = make_ArrayHandleExtractComponent(array, i);
                data->d()->x[i]->setHandle(x);
            }
            break;
        }
#if 0
        case 4: {
            auto data = std::make_shared<vistle::Vec<V, 4>>(array.GetNumberOfValues());
            result = data;
            for (int i = 0; i < numComponents; ++i) {
                auto x = make_ArrayHandleExtractComponent(array, i);
                data->d()->x[i]->setHandle(x);
            }
            break;
        }
#endif
        }
    }
};
} // namespace

DataBase::ptr vtkmFieldToVistle(const vtkm::cont::DataSet &vtkmDataset, const std::string &fieldName)
{
    DataBase::ptr result;
    if (!vtkmDataset.HasField(fieldName))
        return result;

    auto field = vtkmDataset.GetField(fieldName);
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
