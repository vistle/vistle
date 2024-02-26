#include <vistle/core/lines.h>
#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>
#include <vistle/core/spheres.h>
#include <vistle/core/triangles.h>

#include <vtkm/cont/ArrayHandleIndex.h>

#include "cellsetToVtkm.h"

namespace vistle {


CellsetConverter *getCellsetConverter(Object::const_ptr grid)
{
    if (auto structuredGrid = grid->getInterface<StructuredGridBase>()) {
        return new StructuredCellsetConverter(structuredGrid);

    } else if (auto unstructuredGrid = UnstructuredGrid::as(grid)) {
        return new UnstructuredCellsetConverter(unstructuredGrid);

    } else if (auto triangles = Triangles::as(grid)) {
        return new NgonsCellsetConverter<Triangles::const_ptr>(triangles, vtkm::CELL_SHAPE_TRIANGLE, 3);

    } else if (auto quads = Quads::as(grid)) {
        return new NgonsCellsetConverter<Quads::const_ptr>(quads, vtkm::CELL_SHAPE_QUAD, 4);

    } else if (auto polygons = Polygons::as(grid)) {
        return new IndexedCellsetConverter<Polygons::const_ptr>(polygons, vtkm::CELL_SHAPE_POLYGON);

    } else if (auto lines = Lines::as(grid)) {
        return new IndexedCellsetConverter<Lines::const_ptr>(lines, vtkm::CELL_SHAPE_POLY_LINE);

    } else if (auto points = Points::as(grid)) {
        return new PointsCellsetConverter(points);

    } else if (Spheres::as(grid)) {
        // vtkm does not have a specific cell set for spheres. For spheres it is enough to store
        // the coordinate system and add a point field with the radius to the vtkm dataset, so
        // the cell set is simply ignored
        return new SkipConversion(VtkmTransformStatus::SUCCESS);

    } else {
        return new SkipConversion(VtkmTransformStatus::UNSUPPORTED_GRID_TYPE);
    }
}

VtkmTransformStatus cellsetToVtkm(vistle::Object::const_ptr grid, vtkm::cont::DataSet &vtkmDataset)
{
    auto status = getCellsetConverter(grid)->toVtkm(vtkmDataset);
    return status;
}

VtkmTransformStatus StructuredCellsetConverter::toVtkm(vtkm::cont::DataSet &vtkmDataset)
{
    vtkm::Id xDim = m_grid->getNumDivisions(xId);
    vtkm::Id yDim = m_grid->getNumDivisions(yId);
    vtkm::Id zDim = m_grid->getNumDivisions(zId);

    if (zDim > 0) {
        vtkm::cont::CellSetStructured<3> str3;
        str3.SetPointDimensions({xDim, yDim, zDim});
        vtkmDataset.SetCellSet(str3);

    } else if (yDim > 0) {
        vtkm::cont::CellSetStructured<2> str2;
        str2.SetPointDimensions({xDim, yDim});
        vtkmDataset.SetCellSet(str2);

    } else {
        vtkm::cont::CellSetStructured<1> str1;
        str1.SetPointDimensions(xDim);
        vtkmDataset.SetCellSet(str1);
    }
    return VtkmTransformStatus::SUCCESS;
}


VtkmTransformStatus UnstructuredCellsetConverter::toVtkm(vtkm::cont::DataSet &vtkmDataset)
{
    vtkm::cont::CellSetExplicit<> cellSetExplicit;
    cellSetExplicit.Fill(m_grid->getNumCoords(), m_grid->tl().handle(), m_grid->cl().handle(), m_grid->el().handle());

    vtkmDataset.SetCellSet(cellSetExplicit);

    return VtkmTransformStatus::SUCCESS;
}

template<typename NgonsPtr>
VtkmTransformStatus NgonsCellsetConverter<NgonsPtr>::toVtkm(vtkm::cont::DataSet &vtkmDataset)
{
    auto numPoints = m_grid->getNumCoords();
    if (m_grid->getNumCorners() > 0) {
        vtkm::cont::CellSetSingleType<> cellSet;
        auto connectivity = m_grid->cl().handle();
        cellSet.Fill(numPoints, m_cellType, m_pointsPerCell, connectivity);
        vtkmDataset.SetCellSet(cellSet);

    } else {
        vtkm::cont::CellSetSingleType<vtkm::cont::StorageTagCounting> cellSet;
        auto connectivity = vtkm::cont::make_ArrayHandleCounting(
            static_cast<vtkm::Id>(0), static_cast<vtkm::Id>(m_pointsPerCell), m_grid->getNumElements());
        cellSet.Fill(numPoints, m_cellType, m_pointsPerCell, connectivity);
        vtkmDataset.SetCellSet(cellSet);
    }
    return VtkmTransformStatus::SUCCESS;
}


template<typename IndexedPtr>
VtkmTransformStatus IndexedCellsetConverter<IndexedPtr>::toVtkm(vtkm::cont::DataSet &vtkmDataset)
{
    m_grid->check();

    auto shapesc = vtkm::cont::make_ArrayHandleConstant(m_cellType, m_grid->getNumElements());
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
    cellSet.Fill(m_grid->getNumCoords(), shapes, m_grid->cl().handle(), m_grid->el().handle());
    vtkmDataset.SetCellSet(cellSet);

    return VtkmTransformStatus::SUCCESS;
}

VtkmTransformStatus PointsCellsetConverter::toVtkm(vtkm::cont::DataSet &vtkmDataset)
{
    auto numPoints = m_grid->getNumPoints();
    vtkm::cont::CellSetSingleType<vtkm::cont::StorageTagIndex> cellSet;
    cellSet.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, vtkm::cont::make_ArrayHandleIndex(numPoints));
    vtkmDataset.SetCellSet(cellSet);

    return VtkmTransformStatus::SUCCESS;
}

} // namespace vistle
