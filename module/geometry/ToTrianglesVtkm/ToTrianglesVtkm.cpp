#include <viskores/filter/geometry_refinement/Triangulate.h>

#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>
#include <vistle/core/unstr.h>

#include <vistle/vtkm/convert_worklets.h>

#include "ToTrianglesVtkm.h"

MODULE_MAIN(ToTrianglesVtkm)

using namespace vistle;

ToTrianglesVtkm::ToTrianglesVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Use)
{}

template<typename ArrayHandle>
bool cellsArePolygons(const ArrayHandle &cellShapes)
{
    auto [minDim, maxDim] = getMinMaxDims(cellShapes);
    return minDim == 2 && maxDim == 2;
}

ModuleStatusPtr ToTrianglesVtkm::prepareInputGrid(const vistle::Object::const_ptr &grid,
                                                  viskores::cont::DataSet &dataset) const
{
    applyFilter = false;

    if (Polygons::as(grid) || Quads::as(grid)) {
        applyFilter = true;
        return VtkmModule::prepareInputGrid(grid, dataset);

    } else if (auto unstr = UnstructuredGrid::as(grid)) {
        if (cellsArePolygons(unstr->tl().handle())) {
            applyFilter = true;
            return VtkmModule::prepareInputGrid(grid, dataset);
        }
    }

    return Warning("Input grid is not polygonal, skipping triangulation!");
}


std::unique_ptr<viskores::filter::Filter> ToTrianglesVtkm::setUpFilter() const
{
    if (!applyFilter)
        return nullptr;
    return std::make_unique<viskores::filter::geometry_refinement::Triangulate>();
}

vistle::Object::const_ptr ToTrianglesVtkm::prepareOutputGrid(const viskores::cont::DataSet &dataset,
                                                             const vistle::Object::const_ptr &inputGrid) const
{
    if (!applyFilter) {
        auto outputGrid = inputGrid->clone();
        updateMeta(outputGrid);
        outputGrid->copyAttributes(inputGrid);
        return outputGrid;
    }
    return VtkmModule::prepareOutputGrid(dataset, inputGrid);
}

vistle::DataBase::ptr ToTrianglesVtkm::prepareOutputField(const viskores::cont::DataSet &dataset,
                                                          const vistle::Object::const_ptr &inputGrid,
                                                          const vistle::DataBase::const_ptr &inputField,
                                                          const std::string &fieldName,
                                                          const vistle::Object::const_ptr &outputGrid) const
{
    if (!applyFilter) {
        auto outputField = inputField->clone();
        updateMeta(outputField);
        outputField->copyAttributes(inputField);
        if (outputGrid)
            outputField->setGrid(outputGrid);
        return outputField;
    }
    return VtkmModule::prepareOutputField(dataset, inputGrid, inputField, fieldName, outputGrid);
}
