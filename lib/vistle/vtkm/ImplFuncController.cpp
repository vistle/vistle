#include "ImplFuncController.h"

using namespace vistle;

ImplFuncController::ImplFuncController(vistle::Module *module): m_module(module), m_option(nullptr)
{}

void ImplFuncController::init()
{
    m_module->addVectorParameter("point", "point on plane", ParamVector(0.0, 0.0, 0.0));
    m_module->addVectorParameter("vertex", "normal on plane", ParamVector(0.0, 1.0, 0.0));
    m_module->addFloatParameter("scalar", "distance to origin of ordinates", 0.0);
    m_option = m_module->addIntParameter("option", "option", Plane, Parameter::Choice);
    m_module->setParameterChoices(m_option, valueList((ImplFuncOption)0));
    m_module->addVectorParameter("direction", "direction for variable Cylinder", ParamVector(0.0, 0.0, 0.0));
}

bool ImplFuncController::changeParameter(const vistle::Parameter *param)
{
    switch (m_option->getValue()) {
    case Plane: {
        if (param->getName() == "option" || param->getName() == "point") {
            Vector3 vertex = m_module->getVectorParameter("vertex");
            Vector3 point = m_module->getVectorParameter("point");
            m_module->setFloatParameter("scalar", point.dot(vertex));
        }
        return true;
    }
    case Sphere: {
        if (param->getName() == "option" || param->getName() == "point") {
            Vector3 vertex = m_module->getVectorParameter("vertex");
            Vector3 point = m_module->getVectorParameter("point");
            Vector3 diff = vertex - point;
            m_module->setFloatParameter("scalar", diff.norm());
        }
        return true;
    }
    case Box: {
        if (param->getName() == "option" || param->getName() == "point") {
            Vector3 vertex = m_module->getVectorParameter("vertex");
            Vector3 point = m_module->getVectorParameter("point");
            Vector3 diff = vertex - point;
            m_module->setFloatParameter("scalar", diff.norm());
        }
        return true;
    }
    default: /*cylinders*/ {
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
        if (param->getName() == "option" || param->getName() == "point") {
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
