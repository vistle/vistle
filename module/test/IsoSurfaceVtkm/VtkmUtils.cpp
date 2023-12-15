#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/CellSetExplicit.h>

#include <vistle/core/scalars.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>

#include <boost/mpl/for_each.hpp>

#include "VtkmUtils.h"


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
        auto numPoints = coords->getNumCoords();
        auto xCoords = coords->x();
        auto yCoords = coords->y();
        auto zCoords = coords->z();

        auto coordinateSystem = vtkm::cont::CoordinateSystem(
            "coordinate system",
            vtkm::cont::make_ArrayHandleSOA(coords->z().handle(), coords->y().handle(), coords->x().handle()));

        vtkmDataset.AddCoordinateSystem(coordinateSystem);
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
        auto nx = rect->getNumDivisions(0);
        auto ny = rect->getNumDivisions(1);
        auto nz = rect->getNumDivisions(2);
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
        auto numConn = indexedGrid->getNumCorners();

        auto numElements = indexedGrid->getNumElements();

        auto typeList = &unstructuredGrid->tl()[0];

        auto conn = indexedGrid->cl().handle();
        auto offs = indexedGrid->el().handle();
        auto shapes = unstructuredGrid->tl().handle();

        vtkm::cont::CellSetExplicit<> cellSetExplicit;
        cellSetExplicit.Fill(numPoints, shapes, conn, offs);

        // create vtkm dataset
        vtkmDataset.SetCellSet(cellSetExplicit);
    } else {
        return VtkmTransformStatus::UNSUPPORTED_GRID_TYPE;
    }

    return VtkmTransformStatus::SUCCESS;
}

struct AddField {
    vtkm::cont::DataSet &dataset;
    DataBase::const_ptr &object;
    const std::string &name;
    bool &handled;
    AddField(vtkm::cont::DataSet &ds, DataBase::const_ptr obj, const std::string &name, bool &handled)
    : dataset(ds), object(obj), name(name), handled(handled)
    {}
    template<typename S>
    void operator()(S)
    {
        typedef Vec<S, 1> V1;
        typedef Vec<S, 2> V2;
        typedef Vec<S, 3> V3;
        typedef Vec<S, 4> V4;

        Index size = object->getSize();
        auto mapping = object->guessMapping();
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

Triangles::ptr vtkmIsosurfaceToVistleTriangles(vtkm::cont::DataSet &isosurface)
{
    // we expect vtkm isosurface to be a dataset with a cellset only made of triangles
    assert((isosurface.GetCellSet().CanConvert<vtkm::cont::CellSetSingleType<>>() &&
            isosurface.GetCellSet().GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE));
    auto isoGrid = isosurface.GetCellSet().AsCellSet<vtkm::cont::CellSetSingleType<>>();

    // get vertices that make up the isosurface grid
    auto uPointCoordinates = isosurface.GetCoordinateSystem().GetData();

    // we expect point coordinates to be stored as vtkm::Vec3 array handle
    assert((uPointCoordinates.CanConvert<vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>>() == true));
    auto pointCoordinates =
        uPointCoordinates.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>>();
    auto pointsPortal = pointCoordinates.ReadPortal();
    auto numPoints = isoGrid.GetNumberOfPoints();

    // get connectivity array of the isosurface
    auto connectivity = isoGrid.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    auto connPortal = connectivity.ReadPortal();
    auto numConn = connectivity.GetNumberOfValues();

    // and store them as vistle Triangles object
    Triangles::ptr isoTriangles(new Triangles(numConn, numPoints));

    for (vtkm::Id index = 0; index < numPoints; index++) {
        vtkm::Vec3f point = pointsPortal.Get(index);
        // account for coordinate axes swap
        isoTriangles->x()[index] = point[2];
        isoTriangles->y()[index] = point[1];
        isoTriangles->z()[index] = point[0];
    }

    for (vtkm::Id index = 0; index < numConn; index++) {
        isoTriangles->cl()[index] = connPortal.Get(index);
    }

    return isoTriangles;
}

template<typename C>
struct Vistle;

#define MAP(vtkm_t, vistle_t) \
    template<> \
    struct Vistle<vtkm_t> { \
        typedef vistle_t type; \
    }

MAP(vtkm::Int8, vistle::Byte);
MAP(vtkm::UInt8, vistle::Index);
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
    if (field.IsCellField() && !field.IsPointField())
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
