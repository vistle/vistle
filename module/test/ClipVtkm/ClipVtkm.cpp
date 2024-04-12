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


ImplFuncController::ImplFuncController(vistle::Module *module): m_module(module), m_option(nullptr), m_flip(nullptr)
{}

void ImplFuncController::init()
{
    m_module->addVectorParameter("point", "point on plane", ParamVector(0.0, 1.0, 0.0));
    m_module->addVectorParameter("vertex", "normal on plane", ParamVector(0.0, 0.0, 0.0));
    m_module->addFloatParameter("scalar", "distance to origin of ordinates", 0.0);
    m_option = m_module->addIntParameter("option", "option", Plane, Parameter::Choice);
    m_module->setParameterChoices(m_option, valueList((ImplFuncOption)0));
    m_module->addVectorParameter("direction", "direction for variable Cylinder", ParamVector(0.0, 0.0, 0.0));
    m_flip = m_module->addIntParameter("flip", "flip inside out", false, Parameter::Boolean);
}

bool ImplFuncController::changeParameter(const vistle::Parameter *param)
{
    switch (m_option->getValue()) {
    case Plane: {
        if (param->getName() == "point") {
            Vector3 vertex = m_module->getVectorParameter("vertex");
            Vector3 point = m_module->getVectorParameter("point");
            m_module->setFloatParameter("scalar", point.dot(vertex));
        }
        return true;
    }
    case Sphere: {
        if (param->getName() == "point") {
            Vector3 vertex = m_module->getVectorParameter("vertex");
            Vector3 point = m_module->getVectorParameter("point");
            Vector3 diff = vertex - point;
            m_module->setFloatParameter("scalar", diff.norm());
        }
        return true;
    }
    case Box: {
        if (param->getName() == "point") {
            Vector3 vertex = m_module->getVectorParameter("vertex");
            Vector3 point = m_module->getVectorParameter("point");
            Vector3 diff = vertex - point;
            m_module->setFloatParameter("scalar", diff.norm());
        }
        return true;
    }
    default: /*cylinders*/ {
        if (param->getName() == "option") {
            switch (m_option->getValue()) {
            case CylinderX:
                m_module->setVectorParameter("direction", ParamVector(1, 0, 0));
                break;
            case CylinderY:
                m_module->setVectorParameter("direction", ParamVector(0, 1, 0));
                break;
            case CylinderZ:
                m_module->setVectorParameter("direction", ParamVector(0, 0, 1));
                break;
            }
        }
        if (param->getName() == "point") {
            std::cerr << "point" << std::endl;
            Vector3 vertex = m_module->getVectorParameter("vertex");
            Vector3 point = m_module->getVectorParameter("point");
            Vector3 direction = m_module->getVectorParameter("direction");
            Vector3 diff = direction.cross(vertex - point);
            m_module->setFloatParameter("scalar", diff.norm());
        }
        if (param->getName() == "scalar") {
            std::cerr << "scalar" << std::endl;
            Vector3 vertex = m_module->getVectorParameter("vertex");
            Vector3 point = m_module->getVectorParameter("point");
            Vector3 direction = m_module->getVectorParameter("direction");
            Vector3 diff = direction.cross(vertex - point);
            float scalar = m_module->getFloatParameter("scalar");
            Vector3 newPoint = scalar * diff / diff.norm();
            m_module->setVectorParameter("point", ParamVector(newPoint[0], newPoint[1], newPoint[2]));
        }
        return true;
    }
    }

    return false;
}

bool ImplFuncController::flip() const
{
    return m_flip->getValue() != 0;
}

vtkm::ImplicitFunctionGeneral ImplFuncController::func() const
{
    Vector3 pvertex = m_module->getVectorParameter("vertex");
    Vector3 ppoint = m_module->getVectorParameter("point");
    Scalar scalar = m_module->getFloatParameter("scalar");
    Vector3 direction = m_module->getVectorParameter("direction");
    direction.normalize();

    auto vertex = vtkm::make_Vec(pvertex[0], pvertex[1], pvertex[2]);
    auto point = vtkm::make_Vec(ppoint[0], ppoint[1], ppoint[2]);
    auto axis = vtkm::make_Vec(direction[0], direction[1], direction[2]);

    auto pmin =
        vtkm::make_Vec(std::min(vertex[0], point[0]), std::min(vertex[1], point[1]), std::min(vertex[2], point[2]));
    auto pmax =
        vtkm::make_Vec(std::max(vertex[0], point[0]), std::max(vertex[1], point[1]), std::max(vertex[2], point[2]));

    switch (m_option->getValue()) {
    case Plane:
        return vtkm::Plane(point, vertex);
    case Sphere:
        return vtkm::Sphere(vertex, scalar);
    case Box:
        return vtkm::Box(pmin, pmax);
    case CylinderX:
    case CylinderY:
    case CylinderZ:
        return vtkm::Cylinder(vertex, axis, scalar);
    }

    m_module->sendError("unsupported option");
    return vtkm::Plane(vtkm::make_Vec(0, 0, 0), vtkm::make_Vec(0, 0, 1));
}


ClipVtkm::ClipVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm), isocontrol(this)
{
    setDefaultCacheMode(ObjectCache::CacheDeleteLate);

    createInputPort("grid_in", "input grid or geometry with optional data");
    m_dataOut = createOutputPort("grid_out", "surface with mapped data");

    isocontrol.init();
}

ClipVtkm::~ClipVtkm()
{}

bool ClipVtkm::changeParameter(const Parameter *param)
{
    bool ok = isocontrol.changeParameter(param);
    return Module::changeParameter(param) && ok;
}


bool ClipVtkm::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    // make sure input data is supported
    auto isoData = task->expect<Object>("grid_in");
    if (!isoData) {
        sendError("need geometry on grid_in");
        return true;
    }
    auto splitIso = splitContainerObject(isoData);
    auto grid = splitIso.geometry;
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

    // apply vtkm isosurface filter
    std::string isospecies;
    vtkm::filter::contour::ClipWithImplicitFunction isosurfaceFilter;
    auto isoField = splitIso.mapped;
    if (isoField) {
        isospecies = isoField->getAttribute("_species");
        if (isospecies.empty())
            isospecies = "isodata";
        status = vtkmAddField(vtkmDataSet, isoField, isospecies);
        if (status == VtkmTransformStatus::UNSUPPORTED_FIELD_TYPE) {
            sendError("Unsupported iso field type");
            return true;
        }
        isosurfaceFilter.SetActiveField(isospecies);
    }
    isosurfaceFilter.SetImplicitFunction(isocontrol.func());
    isosurfaceFilter.SetInvertClip(isocontrol.flip());
    auto isosurface = isosurfaceFilter.Execute(vtkmDataSet);

    // transform result back into vistle format
    Object::ptr geoOut = vtkmGetGeometry(isosurface);
    if (geoOut) {
        updateMeta(geoOut);
        geoOut->copyAttributes(isoField);
        geoOut->copyAttributes(grid, false);
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

    if (isoField) {
        if (auto mapped = vtkmGetField(isosurface, isospecies)) {
            std::cerr << "mapped data: " << *mapped << std::endl;
            mapped->copyAttributes(isoField);
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
