#include <vistle/alg/objalg.h>
#include <vistle/core/triangles.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/util/stopwatch.h>

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/filter/contour/ClipWithImplicitFunction.h>

#include "ClipVtkm.h"
#include <vistle/vtkm/convert.h>

using namespace vistle;

MODULE_MAIN(ClipVtkm)

ClipVtkm::ClipVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm), m_implFuncControl(this)
{
    createInputPort("grid_in", "input grid or geometry with optional data");
    m_dataOut = createOutputPort("grid_out", "surface with mapped data");

    m_implFuncControl.init();
}

ClipVtkm::~ClipVtkm()
{}

bool ClipVtkm::changeParameter(const Parameter *param)
{
    bool ok = m_implFuncControl.changeParameter(param);
    return Module::changeParameter(param) && ok;
}


bool ClipVtkm::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    // make sure input data is supported
    auto inObj = task->expect<Object>("grid_in");
    if (!inObj) {
        sendError("need geometry on grid_in");
        return true;
    }
    auto inSplit = splitContainerObject(inObj);
    auto grid = inSplit.geometry;
    if (!grid) {
        sendError("no grid on scalar input data");
        return true;
    }

    // transform vistle dataset to vtkm dataset
    vtkm::cont::DataSet vtkmDataSet;
    auto status = vtkmSetGrid(vtkmDataSet, grid);
    if (status == VtkmTransformStatus::UNSUPPORTED_GRID_TYPE) {
        sendError("Currently only supporting unstructured grids");
        return true;
    } else if (status == VtkmTransformStatus::UNSUPPORTED_CELL_TYPE) {
        sendError("Can only transform these cells from vistle to vtkm: point, bar, triangle, polygon, quad, tetra, "
                  "hexahedron, pyramid");
        return true;
    }

    // apply vtkm clip filter
    std::string mapSpecies;
    vtkm::filter::contour::ClipWithImplicitFunction clipFilter;
    auto mapField = inSplit.mapped;
    if (mapField) {
        mapSpecies = mapField->getAttribute("_species");
        if (mapSpecies.empty())
            mapSpecies = "mapdata";
        status = vtkmAddField(vtkmDataSet, mapField, mapSpecies);
        if (status == VtkmTransformStatus::UNSUPPORTED_FIELD_TYPE) {
            sendError("Unsupported mapped field type");
            return true;
        }
        clipFilter.SetActiveField(mapSpecies);
    }
    clipFilter.SetImplicitFunction(m_implFuncControl.func());
    clipFilter.SetInvertClip(m_implFuncControl.flip());
    auto clipped = clipFilter.Execute(vtkmDataSet);

    // transform result back into vistle format
    Object::ptr geoOut = vtkmGetGeometry(clipped);
    if (geoOut) {
        updateMeta(geoOut);
        geoOut->copyAttributes(grid);
        geoOut->setTransform(grid->getTransform());
        if (geoOut->getTimestep() < 0) {
            geoOut->setTimestep(grid->getTimestep());
            geoOut->setNumTimesteps(grid->getNumTimesteps());
        }
        if (geoOut->getBlock() < 0) {
            geoOut->setBlock(grid->getBlock());
            geoOut->setNumBlocks(grid->getNumBlocks());
        }
    }

    if (mapField) {
        if (auto mapped = vtkmGetField(clipped, mapSpecies)) {
            std::cerr << "mapped data: " << *mapped << std::endl;
            mapped->copyAttributes(mapField);
            mapped->setGrid(geoOut);
            updateMeta(mapped);
            task->addObject(m_dataOut, mapped);
            return true;
        } else {
            sendError("could not handle mapped data");
            task->addObject(m_dataOut, geoOut);
        }
    } else {
        task->addObject(m_dataOut, geoOut);
    }

    return true;
}
