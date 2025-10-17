#include "IsoDataFunctor.h"
#include <vistle/module/module.h>

using namespace vistle;

IsoController::IsoController(vistle::Module *module)
#ifdef CUTTINGSURFACE
: m_module(module)
, m_option(nullptr)
#ifdef TOGGLESIGN
, m_flip(nullptr)
#endif
#endif
{}

void IsoController::init()
{
#ifdef CUTTINGSURFACE
    m_module->addVectorParameter("point", "point on plane", ParamVector(0.0, 0.0, 0.0));
    m_module->addVectorParameter("vertex", "normal on plane", ParamVector(0.0, 1.0, 0.0));
    m_module->addFloatParameter("scalar", "distance to origin of ordinates", 0.0);
    m_option = m_module->addIntParameter("option", "option", Plane, Parameter::Choice);
    m_module->setParameterChoices(m_option, valueList((SurfaceOption)0));
    m_module->addVectorParameter("direction", "direction for variable Cylinder", ParamVector(0.0, 0.0, 0.0));
#ifdef TOGGLESIGN
    m_flip = m_module->addIntParameter("flip", "flip inside out", false, Parameter::Boolean);
#endif
#else
#endif
}

bool IsoController::changeParameter(const vistle::Parameter *param)
{
#ifdef CUTTINGSURFACE
    switch (m_option->getValue()) {
    case Plane: {
        m_module->setParameterReadOnly("direction", true);
        if (param->getName() == "point") {
            Vector3 vertex = m_module->getVectorParameter("vertex");
            Vector3 point = m_module->getVectorParameter("point");
            m_module->setFloatParameter("scalar", point.dot(vertex));
        }
        return true;
    }
    case Sphere: {
        m_module->setParameterReadOnly("direction", true);
        if (param->getName() == "point") {
            Vector3 vertex = m_module->getVectorParameter("vertex");
            Vector3 point = m_module->getVectorParameter("point");
            Vector3 diff = vertex - point;
            m_module->setFloatParameter("scalar", diff.norm());
        }
        return true;
    }
    default: /*cylinders*/ {
        m_module->setParameterReadOnly("direction", false);
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
#else
#endif

    return false;
}

#ifdef CUTTINGSURFACE
IsoDataFunctor IsoController::newFunc(const vistle::Matrix4 &objTransform, const vistle::Scalar *x,
                                      const vistle::Scalar *y, const vistle::Scalar *z) const
{
    Vector3 vertex = m_module->getVectorParameter("vertex");
    Vector3 point = m_module->getVectorParameter("point");
    Vector3 direction = m_module->getVectorParameter("direction");
    auto inv = objTransform.inverse();
    auto normalTransform = inv.block<3, 3>(0, 0).inverse().transpose();
    vertex = normalTransform * vertex;
    point = transformPoint(inv, point);

#ifdef TOGGLESIGN
    return IsoDataFunctor(vertex, point, direction, x, y, z, m_option->getValue(), m_flip->getValue());
#else
    return IsoDataFunctor(vertex, point, direction, x, y, z, m_option->getValue());
#endif
}

IsoDataFunctor IsoController::newFunc(const vistle::Matrix4 &objTransform, const vistle::Index dims[3],
                                      const vistle::Scalar *x, const vistle::Scalar *y, const vistle::Scalar *z) const
{
    Vector3 vertex = m_module->getVectorParameter("vertex");
    Vector3 point = m_module->getVectorParameter("point");
    Vector3 direction = m_module->getVectorParameter("direction");
    auto inv = objTransform.inverse();
    auto normalTransform = inv.block<3, 3>(0, 0).inverse().transpose();
    vertex = normalTransform * vertex;
    point = transformPoint(inv, point);

#ifdef TOGGLESIGN
    return IsoDataFunctor(vertex, point, direction, dims, x, y, z, m_option->getValue(), m_flip->getValue());
#else
    return IsoDataFunctor(vertex, point, direction, dims, x, y, z, m_option->getValue());
#endif
}
#else
IsoDataFunctor IsoController::newFunc(const vistle::Matrix4 &objTransform, const vistle::Scalar *data) const
{
    return IsoDataFunctor(data);
}
#endif
