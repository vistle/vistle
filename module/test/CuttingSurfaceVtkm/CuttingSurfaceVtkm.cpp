#include <vtkm/cont/DataSet.h>

#include <vtkm/filter/contour/Slice.h>

#include <vistle/alg/objalg.h>
#include <vistle/vtkm/convert.h>

#include "CuttingSurfaceVtkm.h"

MODULE_MAIN(CuttingSurfaceVtkm)

using namespace vistle;

// order has to match OpenCOVER's CuttingSurfaceInteraction
DEFINE_ENUM_WITH_STRING_CONVERSIONS(SurfaceOption, (Plane)(Sphere)(CylinderX)(CylinderY)(CylinderZ))

CuttingSurfaceVtkm::CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    // ports
    m_mapDataIn = createInputPort("data_in", "input grid or geometry with mapped data");
    m_dataOut = createOutputPort("data_out", "surface with mapped data");

    // parameters
    m_point = addVectorParameter("point", "point on plane", ParamVector(0.0, 0.0, 0.0));
    m_vertex = addVectorParameter("vertex", "normal on plane", ParamVector(1.0, 0.0, 0.0));
    m_scalar = addFloatParameter("scalar", "distance to origin of ordinates", 0.0);
    m_option = addIntParameter("option", "option", Plane, Parameter::Choice);
    setParameterChoices(m_option, valueList((SurfaceOption)0));

    m_direction = addVectorParameter("direction", "direction for variable Cylinder", ParamVector(0.0, 0.0, 0.0));
    m_computeNormals =
        addIntParameter("compute_normals", "compute normals (structured grids only)", 0, Parameter::Boolean);
}

CuttingSurfaceVtkm::~CuttingSurfaceVtkm()
{}

vtkm::ImplicitFunctionGeneral CuttingSurfaceVtkm::getImplicitFunction() const
{
    vtkm::Vec<vistle::Float, 3> point = {m_point->getValue()[0], m_point->getValue()[1], m_point->getValue()[2]};
    vtkm::Vec<vistle::Float, 3> vector = {m_vertex->getValue()[0], m_vertex->getValue()[1], m_vertex->getValue()[2]};

    switch (m_option->getValue()) {
    case Plane:
        return vtkm::Plane(point, vector);
    case Sphere:
        return vtkm::Sphere(point, m_scalar->getValue());
    case CylinderX:
        return vtkm::Cylinder(point, {1, 0, 0}, m_scalar->getValue());
    case CylinderY:
        return vtkm::Cylinder(point, {0, 1, 0}, m_scalar->getValue());
    case CylinderZ:
        return vtkm::Cylinder(point, {0, 0, 1}, m_scalar->getValue());
    default:
        throw vistle::exception("unknown option");
    }
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

    vtkm::ImplicitFunctionGeneral implicitFunction;

    // apply vtk-m filter
    vtkm::filter::contour::Slice sliceFilter;
    sliceFilter.SetImplicitFunction(getImplicitFunction());
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
