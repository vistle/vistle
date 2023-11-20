#include <limits>

#include <vistle/core/unstr.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/CellSetExplicit.h>

#include "VtkmUtils.h"

using namespace vistle;

// helper function to transform vistle cell types into vtkm cell types
vtkm::cont::ArrayHandle<vtkm::UInt8> vistleTypeListToVtkmShapes(Index numElements, const Byte *typeList)
{
    vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
    shapes.Allocate(numElements);
    auto shapesPortal = shapes.WritePortal();
    for (unsigned int i = 0; i < shapes.GetNumberOfValues(); i++) {
        if (typeList[i] == cell::POINT)
            shapesPortal.Set(i, vtkm::CELL_SHAPE_VERTEX);
        else if (typeList[i] == cell::BAR)
            shapesPortal.Set(i, vtkm::CELL_SHAPE_LINE);
        else if (typeList[i] == cell::TRIANGLE)
            shapesPortal.Set(i, vtkm::CELL_SHAPE_TRIANGLE);
        else if (typeList[i] == cell::POLYGON)
            shapesPortal.Set(i, vtkm::CELL_SHAPE_POLYGON);
        else if (typeList[i] == cell::QUAD)
            shapesPortal.Set(i, vtkm::CELL_SHAPE_QUAD);
        else if (typeList[i] == cell::TETRAHEDRON)
            shapesPortal.Set(i, vtkm::CELL_SHAPE_TETRA);
        else if (typeList[i] == cell::HEXAHEDRON)
            shapesPortal.Set(i, vtkm::CELL_SHAPE_HEXAHEDRON);
        else if (typeList[i] == cell::PRISM)
            shapesPortal.Set(i, vtkm::CELL_SHAPE_WEDGE);
        else if (typeList[i] == cell::PYRAMID)
            shapesPortal.Set(i, vtkm::CELL_SHAPE_PYRAMID);
        else {
            break;
        }
    }
    return shapes;
}

VTKM_TRANSFORM_STATUS vistleToVtkmDataSet(vistle::Object::const_ptr grid,
                                          std::shared_ptr<const vistle::Vec<Scalar, 1U>> scalarField,
                                          vtkm::cont::DataSet &vtkmDataset)
{
    auto indexedGrid = Indexed::as(grid);
    auto unstructuredGrid = UnstructuredGrid::as(indexedGrid);
    if (!unstructuredGrid)
        return VTKM_TRANSFORM_STATUS::UNSUPPORTED_GRID_TYPE;

    auto points = Vec<Scalar, 3>::as(grid);
    auto numPoints = indexedGrid->getNumCoords();

    auto xCoords = points->x();
    auto yCoords = points->y();
    auto zCoords = points->z();

    auto numConn = indexedGrid->getNumCorners();
    auto connList = &indexedGrid->cl()[0];

    // For indexing, vistle uses unsigned integers, while vtk-m uses signed integers.
    // Since in CMakeLists.txt we enforce that both programs use the same number of
    // bits to represent integers, not all ints in vistle can be cast to ints in vtkm.
    if (numConn > std::numeric_limits<vtkm::Id>::max())
        return VTKM_TRANSFORM_STATUS::EXCEEDING_VTKM_ID_LIMIT;

    auto numElements = indexedGrid->getNumElements();
    auto elementList = &indexedGrid->el()[0];

    auto typeList = &unstructuredGrid->tl()[0];

    // points are stored as structures of arrays in vistle, while vtk-m expects arrays of stuctures
    auto coordinateSystem = vtkm::cont::CoordinateSystem(
        "coordinate system",
        vtkm::cont::make_ArrayHandleSOA(vtkm::cont::make_ArrayHandle(xCoords, numPoints, vtkm::CopyFlag::Off),
                                        vtkm::cont::make_ArrayHandle(yCoords, numPoints, vtkm::CopyFlag::Off),
                                        vtkm::cont::make_ArrayHandle(zCoords, numPoints, vtkm::CopyFlag::Off)));

    vtkm::cont::CellSetExplicit<> cellSetExplicit;

    // connectivity list and element list only need type cast (vistle::Index -> vtkm::Id)
    auto connectivity =
        vtkm::cont::make_ArrayHandle(reinterpret_cast<const vtkm::Id *>(connList), numConn, vtkm::CopyFlag::Off);
    auto offsets = vtkm::cont::make_ArrayHandle(reinterpret_cast<const vtkm::Id *>(elementList), numElements + 1,
                                                vtkm::CopyFlag::Off);

    // vistle and vtkm both use their own enums for storing the different cell types
    auto shapes = vistleTypeListToVtkmShapes(numElements, typeList);
    if (shapes.GetNumberOfValues() == 0)
        return VTKM_TRANSFORM_STATUS::UNSUPPORTED_CELL_TYPE;

    cellSetExplicit.Fill(numPoints, shapes, connectivity, offsets);

    // create vtkm dataset
    vtkmDataset.AddCoordinateSystem(coordinateSystem);
    vtkmDataset.SetCellSet(cellSetExplicit);

    vtkmDataset.AddPointField(scalarField->getName(),
                              vtkm::cont::make_ArrayHandle(&scalarField->x()[0], numPoints, vtkm::CopyFlag::Off));

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
