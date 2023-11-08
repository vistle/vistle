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

    auto numElements = indexedGrid->getNumElements();
    auto elementList = &indexedGrid->el()[0];

    auto typeList = &unstructuredGrid->tl()[0];

    // two options for testing: using array handles or std::vectors to create vtkm dataset
    if (useArrayHandles) {
        auto coordinateSystem = vtkm::cont::CoordinateSystem(
            "coordinate system",
            vtkm::cont::make_ArrayHandleSOA(vtkm::cont::make_ArrayHandle(xCoords, numPoints, vtkm::CopyFlag::Off),
                                            vtkm::cont::make_ArrayHandle(yCoords, numPoints, vtkm::CopyFlag::Off),
                                            vtkm::cont::make_ArrayHandle(zCoords, numPoints, vtkm::CopyFlag::Off)));

        vtkm::cont::CellSetExplicit<> cellSetExplicit;

        // connectivity list and element list only need type cast (vistle::Index -> vtkm::Id)
        auto connectivityCast = vtkm::cont::make_ArrayHandleCast<vtkm::Id>(
            vtkm::cont::make_ArrayHandle(connList, numConn, vtkm::CopyFlag::Off));
        // cellSetExplicit.Fill does not accept array handles of type vtkm::cont::ArrayHandleCast
        // so we need to copy here
        vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
        vtkm::cont::ArrayCopyShallowIfPossible(connectivityCast, connectivity);

        auto offsetsCast = vtkm::cont::make_ArrayHandleCast<vtkm::Id>(
            vtkm::cont::make_ArrayHandle(elementList, numElements + 1, vtkm::CopyFlag::Off));
        vtkm::cont::ArrayHandle<vtkm::Id> offsets;
        vtkm::cont::ArrayCopyShallowIfPossible(offsetsCast, offsets);

        // vistle and vtkm both use their own enums for storing the different cell types
        auto shapes = vistleTypeListToVtkmShapes(numElements, typeList);
        if (shapes.GetNumberOfValues() == 0)
            return VTKM_TRANSFORM_STATUS::UNSUPPORTED_CELL_TYPE;

        cellSetExplicit.Fill(numPoints, shapes, connectivity, offsets);

        // create vtkm dataset
        vtkmDataset.AddCoordinateSystem(coordinateSystem);
        vtkmDataset.SetCellSet(cellSetExplicit);

    } else {
        // for the vertices' coordinates vistle uses structures of arrays, while vtkm
        // uses arrays of structures for the points' coordinates
        std::vector<vtkm::Vec3f_32> pointCoordinates;
        for (unsigned int i = 0; i < numPoints; i++)
            pointCoordinates.push_back(vtkm::Vec3f_32(xCoords[i], yCoords[i], zCoords[i]));

        std::vector<vtkm::Id> connectivity(numConn);
        for (unsigned int i = 0; i < connectivity.size(); i++)
            connectivity[i] = static_cast<vtkm::Id>(connList[i]);

        // to make clear at which index of the connectivity list the description of a
        // cell starts, vistle implicitly stores these start indices, while vtkm stores
        // the number of vertices that make up each cell in a list instead
        std::vector<vtkm::IdComponent> numIndices(numElements);
        for (unsigned int i = 0; i < numElements - 1; i++)
            numIndices[i] = elementList[i + 1] - elementList[i];
        numIndices.back() = numConn - elementList[numElements - 1];

        auto shapes = vistleTypeListToVtkmShapesVector(numElements, typeList);
        if (shapes.empty()) {
            return VTKM_TRANSFORM_STATUS::UNSUPPORTED_CELL_TYPE;
        }

        // create vtkm dataset
        vtkm::cont::DataSetBuilderExplicit unstructuredDatasetBuilder;
        vtkmDataset = unstructuredDatasetBuilder.Create(pointCoordinates, shapes, numIndices, connectivity);
    }
    // add scalar field
    vtkmDataset.AddPointField(scalarField->getName(),
                              std::vector<float>(scalarField->x(), scalarField->x() + numPoints));

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
