#ifndef CELLSET_TO_VTKM_H
#define CELLSET_TO_VTKM_H

#include <vistle/core/points.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/unstr.h>

#include <vtkm/cont/DataSet.h>

#include "transformEnums.h"

namespace vistle {

// interface for converting vistle cellsets into vtk-m cellsets
class CellsetConverter {
public:
    virtual VtkmTransformStatus toVtkm(vtkm::cont::DataSet &vtkmDataset) = 0;
};

// determines cellset type of `grid` and creates corresponding `CellsetConverter`
CellsetConverter *getCellsetConverter(Object::const_ptr grid);

// converts the given vistle cellset `grid` to a vtk-m cellset and adds the result to `vtkmDataset`
VtkmTransformStatus cellsetToVtkm(vistle::Object::const_ptr grid, vtkm::cont::DataSet &vtkmDataset);

class StructuredCellsetConverter: public CellsetConverter {
public:
    StructuredCellsetConverter(const StructuredGridBase *grid): m_grid(grid) {}

private:
    const StructuredGridBase *m_grid;

    VtkmTransformStatus toVtkm(vtkm::cont::DataSet &vtkmDataset);
};

class UnstructuredCellsetConverter: public CellsetConverter {
public:
    UnstructuredCellsetConverter(UnstructuredGrid::const_ptr grid): m_grid(grid) {}

private:
    UnstructuredGrid::const_ptr m_grid;

    VtkmTransformStatus toVtkm(vtkm::cont::DataSet &vtkmDataset);
};

template<typename NgonsPtr>
class NgonsCellsetConverter: public CellsetConverter {
public:
    NgonsCellsetConverter(NgonsPtr grid, vtkm::UInt8 cellType, Index pointsPerCell)
    : m_grid(grid), m_cellType(cellType), m_pointsPerCell(pointsPerCell){};

private:
    NgonsPtr m_grid;
    vtkm::UInt8 m_cellType;
    Index m_pointsPerCell;

    VtkmTransformStatus toVtkm(vtkm::cont::DataSet &vtkmDataset);
};

template<typename IndexedPtr>
class IndexedCellsetConverter: public CellsetConverter {
public:
    IndexedCellsetConverter(IndexedPtr grid, vtkm::UInt8 cellType): m_grid(grid), m_cellType(cellType) {}

private:
    IndexedPtr m_grid;
    vtkm::UInt8 m_cellType;

    VtkmTransformStatus toVtkm(vtkm::cont::DataSet &vtkmDataset);
};

class PointsCellsetConverter: public CellsetConverter {
public:
    PointsCellsetConverter(Points::const_ptr grid): m_grid(grid){};

private:
    Points::const_ptr m_grid;

    VtkmTransformStatus toVtkm(vtkm::cont::DataSet &vtkmDataset);
};

// skips cellset conversion by directly returning the status provided via `m_status`
class SkipConversion: public CellsetConverter {
public:
    SkipConversion(VtkmTransformStatus status): m_status(status) {}

private:
    VtkmTransformStatus m_status;

    VtkmTransformStatus toVtkm(vtkm::cont::DataSet &vtkmDataset) { return m_status; }
};

}; // namespace vistle

#endif // CELLSET_TO_VTKM_H
