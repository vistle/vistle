#include "IsoDataFunctor.h"
#include <module/module.h>

using namespace vistle;

IsoController::IsoController(vistle::Module *module)
#ifdef CUTTINGSURFACE
: m_module(module)
, m_option(nullptr)
#ifdef TOGGLESIGN
, m_flip(nullptr)
#endif
#endif
{
#ifdef CUTTINGSURFACE
   m_module->addVectorParameter("point", "point on plane", ParamVector(0.0, 0.0, 0.0));
   m_module->addVectorParameter("vertex", "normal on plane", ParamVector(1.0, 0.0, 0.0));
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

bool IsoController::changeParameter(const vistle::Parameter* param) {

#ifdef CUTTINGSURFACE
   switch (m_option->getValue()) {
      case Plane: {
         if (param->getName() == "point") {
            Vector vertex = m_module->getVectorParameter("vertex");
            Vector point = m_module->getVectorParameter("point");
            m_module->setFloatParameter("scalar",point.dot(vertex));
         }
         return true;
      }
      case Sphere: {
         if (param->getName() == "point") {
            Vector vertex = m_module->getVectorParameter("vertex");
            Vector point = m_module->getVectorParameter("point");
            Vector diff = vertex - point;
            m_module->setFloatParameter("scalar",diff.norm());
         }
         return true;
      }
      default: /*cylinders*/ {
         if (param->getName() == "option") {
            switch(m_option->getValue()) {

               case CylinderX:
                  m_module->setVectorParameter("direction",ParamVector(1,0,0));
                  break;
               case CylinderY:
                  m_module->setVectorParameter("direction",ParamVector(0,1,0));
                  break;
               case CylinderZ:
                  m_module->setVectorParameter("direction",ParamVector(0,0,1));
                  break;
            }
         }
         if (param->getName() == "point") {
            std::cerr<< "point" << std::endl;
            Vector vertex = m_module->getVectorParameter("vertex");
            Vector point = m_module->getVectorParameter("point");
            Vector direction = m_module->getVectorParameter("direction");
            Vector diff = direction.cross(vertex-point);
            m_module->setFloatParameter("scalar",diff.norm());
         }
         if (param->getName() == "scalar") {
            std::cerr<< "scalar" << std::endl;
            Vector vertex = m_module->getVectorParameter("vertex");
            Vector point = m_module->getVectorParameter("point");
            Vector direction = m_module->getVectorParameter("direction");
            Vector diff = direction.cross(vertex-point);
            float scalar = m_module->getFloatParameter("scalar");
            Vector newPoint = scalar*diff/diff.norm();
            m_module->setVectorParameter("point",ParamVector(newPoint[0], newPoint[1], newPoint[2]));
         }
         return true;
      }
   }
#else
#endif

    return false;
}

#ifdef CUTTINGSURFACE
IsoDataFunctor IsoController::newFunc(const vistle::Matrix4 &objTransform, const vistle::Scalar *x, const vistle::Scalar *y, const vistle::Scalar *z) const {
    Vector vertex = m_module->getVectorParameter("vertex");
    Vector point = m_module->getVectorParameter("point");
    Vector direction = m_module->getVectorParameter("direction");
    auto inv = objTransform.inverse();
    auto normalTransform = inv.block<3,3>(0,0).inverse().transpose();
    vertex = normalTransform * vertex;

    Vector4 p4;
    p4 << point, 1.;
    p4 = inv * p4;
    point = p4.block<3,1>(0,0)/p4[3];

#ifdef TOGGLESIGN
    return IsoDataFunctor(vertex, point, direction, x, y, z, m_option->getValue(), m_flip->getValue());
#else
    return IsoDataFunctor(vertex, point, direction, x, y, z, m_option->getValue());
#endif
}
#else
IsoDataFunctor IsoController::newFunc(const vistle::Matrix4 &objTransform, const vistle::Scalar *data) const {
    return IsoDataFunctor(data);
}
#endif


