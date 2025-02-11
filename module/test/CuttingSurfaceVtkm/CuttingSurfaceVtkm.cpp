#include <vtkm/cont/DataSet.h>

#include <vtkm/filter/contour/Slice.h>

#include <vistle/alg/objalg.h>
#include <vistle/vtkm/convert.h>

#include "CuttingSurfaceVtkm.h"

MODULE_MAIN(CuttingSurfaceVtkm)

using namespace vistle;

CuttingSurfaceVtkm::CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm), m_implFuncControl(this)
{
    // ports
    m_mapDataIn = createInputPort("data_in", "input grid or geometry with mapped data");
    m_dataOut = createOutputPort("data_out", "surface with mapped data");

    // parameters
    m_implFuncControl.init();
    m_computeNormals =
        addIntParameter("compute_normals", "compute normals (structured grids only)", 0, Parameter::Boolean);
}

CuttingSurfaceVtkm::~CuttingSurfaceVtkm()
{}

bool CuttingSurfaceVtkm::changeParameter(const Parameter *param)
{
    bool ok = m_implFuncControl.changeParameter(param);
    return Module::changeParameter(param) && ok;
}

bool CuttingSurfaceVtkm::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    // --------------------- PREPARE INPUT DATA ---------------------
    // check input grid and input field
    auto container = task->accept<Object>(m_mapDataIn);
    auto split = splitContainerObject(container);
    auto mapField = split.mapped;
    if (!mapField) {
        sendError("no mapped data");
        return true;
    }
    auto grid = split.geometry;
    if (!grid) {
        sendError("no grid on input data");
        return true;
    }

    // transform Vistle to VTK-m dataset
    vtkm::cont::DataSet vtkmDataSet;
    auto status = vtkmSetGrid(vtkmDataSet, grid);

    if (!status->isSuccessful()) {
        sendError(status->errorMessage());
        return true;
    }

    std::string mapSpecies;
    if (mapField) {
        mapSpecies = mapField->getAttribute("_species");
        if (mapSpecies.empty())
            mapSpecies = "mapdata";
        status = vtkmAddField(vtkmDataSet, mapField, mapSpecies);
        if (!status->isSuccessful()) {
            sendError(status->errorMessage());
            return true;
        }
    }
    // --------------------- PREPARE INPUT DATA ---------------------

    // --------------------- APPLY VTK-M FILTER ---------------------
    // apply vtk-m filter
    vtkm::filter::contour::Slice sliceFilter;
    sliceFilter.SetImplicitFunction(m_implFuncControl.func());
    sliceFilter.SetMergeDuplicatePoints(false);
    sliceFilter.SetGenerateNormals(m_computeNormals->getValue() != 0);

    if (mapField) {
        sliceFilter.SetActiveField(mapSpecies);
    }

    auto sliceResult = sliceFilter.Execute(vtkmDataSet);

    // --------------------- APPLY VTK-M FILTER ---------------------

    // --------------------- PREPARE OUTPUT DATA ---------------------
    // transform result back to Vistle
    Object::ptr geoOut = vtkmGetGeometry(sliceResult);
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
        if (auto mapped = vtkmGetField(sliceResult, mapSpecies)) {
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
    // --------------------- PREPARE OUTPUT DATA ---------------------

    return true;
}
