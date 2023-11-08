#include <vistle/alg/objalg.h>
#include <vistle/core/triangles.h>
#include <vistle/core/unstr.h>

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/filter/contour/Contour.h>

#include "IsoSurfaceVtkm.h"
#include "VtkmUtils.h"

using namespace vistle;

MODULE_MAIN(IsoSurfaceVtkm)

/*
TODO:
    - support input other than per-vertex mapped data on unstructured grid
    - add second input port from original isosurface module (mapped data)
    - remove use_arrayhandles bool after benchmarking module
*/

IsoSurfaceVtkm::IsoSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("data_in", "input grid or geometry with mapped data");
    m_dataOut = createOutputPort("data_out", "surface with mapped data");

    m_isovalue = addFloatParameter("isovalue", "isovalue", 0.0);

    m_useArrayHandles =
        addIntParameter("use_array_handle", "specify wheter to use array handles to transform vistle to vtkm data",
                        true, Parameter::Boolean);
}

IsoSurfaceVtkm::~IsoSurfaceVtkm()
{}

bool IsoSurfaceVtkm::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    // make sure input data is supported
    auto scalarField = task->expect<Vec<Scalar>>("data_in");
    if (!scalarField) {
        sendError("need scalar data on data_in");
        return true;
    }

    if (scalarField->guessMapping() != DataBase::Vertex) {
        sendError("need per-vertex mapping on data_in");
        return true;
    }

    auto splitScalar = splitContainerObject(scalarField);
    auto grid = splitScalar.geometry;
    if (!grid) {
        sendError("no grid on scalar input data");
        return true;
    }

    // transform vistle dataset to vtkm dataset
    vtkm::cont::DataSet vtkmDataSet;
    auto status = vistleToVtkmDataSet(grid, scalarField, vtkmDataSet, m_useArrayHandles);

    if (status == VTKM_TRANSFORM_STATUS::UNSUPPORTED_GRID_TYPE) {
        sendError("Currently only supporting unstructured grids");
        return true;
    } else if (status == VTKM_TRANSFORM_STATUS::UNSUPPORTED_CELL_TYPE) {
        sendError("Can only transform these cells from vistle to vtkm: point, bar, triangle, polygon, quad, tetra, "
                  "hexahedron, pyramid");
        return true;
    }

    // apply vtkm isosurface filter
    vtkm::filter::contour::Contour isosurfaceFilter;
    isosurfaceFilter.SetActiveField(scalarField->getName());
    isosurfaceFilter.SetIsoValue(m_isovalue->getValue());
    auto isosurface = isosurfaceFilter.Execute(vtkmDataSet);

    // transform result back into vistle format
    Object::ptr geoOut = vtkmIsosurfaceToVistleTriangles(isosurface);
    updateMeta(geoOut);
    
    task->addObject(m_dataOut, geoOut);

    return true;
}