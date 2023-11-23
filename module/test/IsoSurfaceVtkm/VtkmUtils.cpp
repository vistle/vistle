#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/CellSetExplicit.h>

#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>

#include "VtkmUtils.h"

using namespace vistle;

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

VTKM_TRANSFORM_STATUS vistleToVtkmDataSet(vistle::Object::const_ptr grid,
                                          std::shared_ptr<const vistle::Vec<Scalar, 1U>> scalarField,
                                          vtkm::cont::DataSet &vtkmDataset, bool useArrayHandles)
{
    if (auto coords = Coords::as(grid)) {
        auto numPoints = coords->getNumCoords();
        auto xCoords = coords->x();
        auto yCoords = coords->y();
        auto zCoords = coords->z();

        auto coordinateSystem = vtkm::cont::CoordinateSystem(
            "coordinate system",
            vtkm::cont::make_ArrayHandleSOA(vtkm::cont::make_ArrayHandle(xCoords, numPoints, vtkm::CopyFlag::Off),
                                            vtkm::cont::make_ArrayHandle(yCoords, numPoints, vtkm::CopyFlag::Off),
                                            vtkm::cont::make_ArrayHandle(zCoords, numPoints, vtkm::CopyFlag::Off)));

        vtkmDataset.AddCoordinateSystem(coordinateSystem);
    } else if (auto uni = UniformGrid::as(grid)) {
        auto nx = uni->getNumDivisions(0);
        auto ny = uni->getNumDivisions(1);
        auto nz = uni->getNumDivisions(2);
        const auto *min = uni->min(), *max = uni->max();
        vtkm::cont::ArrayHandleUniformPointCoordinates uniformCoordinates(
            vtkm::Id3(nx, ny, nz), vtkm::Vec3f{min[0], min[1], min[2]}, vtkm::Vec3f{max[0], max[1], max[2]});
        auto coordinateSystem = vtkm::cont::CoordinateSystem("uniform", uniformCoordinates);
        vtkmDataset.AddCoordinateSystem(coordinateSystem);
    } else if (auto rect = RectilinearGrid::as(grid)) {
        auto nx = rect->getNumDivisions(0);
        auto ny = rect->getNumDivisions(1);
        auto nz = rect->getNumDivisions(2);
        auto xc = vtkm::cont::make_ArrayHandle(&rect->coords(0)[0], nx, vtkm::CopyFlag::Off);
        auto yc = vtkm::cont::make_ArrayHandle(&rect->coords(1)[0], ny, vtkm::CopyFlag::Off);
        auto zc = vtkm::cont::make_ArrayHandle(&rect->coords(2)[0], nz, vtkm::CopyFlag::Off);

        vtkm::cont::ArrayHandleCartesianProduct rectilinearCoordinates(xc, yc, zc);
        auto coordinateSystem = vtkm::cont::CoordinateSystem("rectilinear", rectilinearCoordinates);
        vtkmDataset.AddCoordinateSystem(coordinateSystem);
    } else {
        return VTKM_TRANSFORM_STATUS::UNSUPPORTED_GRID_TYPE;
    }

    auto indexedGrid = Indexed::as(grid);

    if (auto str = grid->getInterface<StructuredGridBase>()) {
        auto nx = str->getNumDivisions(0);
        auto ny = str->getNumDivisions(1);
        auto nz = str->getNumDivisions(2);
        if (nz > 0) {
            vtkm::cont::CellSetStructured<3> str3;
            str3.SetPointDimensions({nx, ny, nz});
            vtkmDataset.SetCellSet(str3);
        } else if (ny > 0) {
            vtkm::cont::CellSetStructured<2> str2;
            str2.SetPointDimensions({nx, ny});
            vtkmDataset.SetCellSet(str2);
        } else {
            vtkm::cont::CellSetStructured<1> str1;
            str1.SetPointDimensions(nx);
            vtkmDataset.SetCellSet(str1);
        }
    } else if (auto unstructuredGrid = UnstructuredGrid::as(indexedGrid)) {
        auto numPoints = indexedGrid->getNumCoords();
        auto numConn = indexedGrid->getNumCorners();
        auto connList = &indexedGrid->cl()[0];

        auto numElements = indexedGrid->getNumElements();
        auto elementList = &indexedGrid->el()[0];

        auto typeList = &unstructuredGrid->tl()[0];

        // two options for testing: using array handles or std::vectors to create vtkm dataset
        static_assert(sizeof(vistle::Index) == sizeof(vtkm::Id),
                      "VTK-m has to be compiled with Id size matching Vistle's Index size");

        auto conn =
            vtkm::cont::make_ArrayHandle(reinterpret_cast<const vtkm::Id *>(connList), numConn, vtkm::CopyFlag::Off);

        auto offs = vtkm::cont::make_ArrayHandle(reinterpret_cast<const vtkm::Id *>(elementList), numElements + 1,
                                                 vtkm::CopyFlag::Off);

        auto shapes = vtkm::cont::make_ArrayHandle(reinterpret_cast<const vtkm::UInt8 *>(typeList), numElements,
                                                   vtkm::CopyFlag::Off);

        vtkm::cont::CellSetExplicit<> cellSetExplicit;
        cellSetExplicit.Fill(numPoints, shapes, conn, offs);

        // create vtkm dataset
        vtkmDataset.SetCellSet(cellSetExplicit);
    } else {
        return VTKM_TRANSFORM_STATUS::UNSUPPORTED_GRID_TYPE;
    }
    vtkmDataset.AddPointField(
        scalarField->getName(),
        vtkm::cont::make_ArrayHandle(&scalarField->x()[0], scalarField->getSize(), vtkm::CopyFlag::Off));

    return VTKM_TRANSFORM_STATUS::SUCCESS;
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
        isoTriangles->x()[index] = point[0];
        isoTriangles->y()[index] = point[1];
        isoTriangles->z()[index] = point[2];
    }

    for (vtkm::Id index = 0; index < numConn; index++) {
        isoTriangles->cl()[index] = connPortal.Get(index);
    }

    return isoTriangles;
}
