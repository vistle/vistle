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
    auto container = task->accept<Object>(m_mapDataIn);
    auto split = splitContainerObject(container);
    auto mapdata = split.mapped;
    if (!mapdata) {
        sendError("no mapped data");
        return true;
    }
    auto grid = split.geometry;
    if (!grid) {
        sendError("no grid on input data");
        return true;
    }

    auto obj = work(grid);
    task->addObject(m_dataOut, obj);

    return true;
}

vistle::Object::ptr CuttingSurfaceVtkm::work(vistle::Object::const_ptr grid) const
{
    // transform Vistle to VTK-m dataset
    vtkm::cont::DataSet vtkmDataSet;
    auto status = vtkmSetGrid(vtkmDataSet, grid);
    if (status == VtkmTransformStatus::UNSUPPORTED_GRID_TYPE) {
        sendError("Currently only supporting unstructured grids");
        return Object::ptr();
    } else if (status == VtkmTransformStatus::UNSUPPORTED_CELL_TYPE) {
        sendError("Can only transform these cells from vistle to vtkm: point, bar, triangle, polygon, quad, tetra, "
                  "hexahedron, pyramid");
        return Object::ptr();
    }

    // apply vtk-m filter
    vtkm::filter::contour::Slice sliceFilter;
    sliceFilter.SetImplicitFunction(m_implFuncControl.func());
    sliceFilter.SetMergeDuplicatePoints(false);
    sliceFilter.SetGenerateNormals(m_computeNormals->getValue() != 0);
    auto sliceResult = sliceFilter.Execute(vtkmDataSet);

    // transform result back to Vistle
    Object::ptr geoOut = vtkmGetGeometry(sliceResult);
    if (!geoOut) {
        return Object::ptr();
    }

    updateMeta(geoOut);
    geoOut->copyAttributes(grid);
    geoOut->copyAttributes(grid, false);
    geoOut->setTransform(grid->getTransform());

    return geoOut;
}
