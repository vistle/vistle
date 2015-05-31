#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/spheres.h>

#include "Spheres.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(MapMode,
      (Fixed)
      (Radius)
      (Surface)
      (Volume)
)

MODULE_MAIN(ToSpheres)

using namespace vistle;

ToSpheres::ToSpheres(const std::string &shmname, const std::string &name, int moduleID)
   : Module("Spheres", shmname, name, moduleID) {

   createInputPort("grid_in");
   createInputPort("data_in");
   createOutputPort("grid_out");

   auto MaxRad = std::numeric_limits<Scalar>::max();

   m_radius = addFloatParameter("radius", "radius or radius scale factor of spheres", 1.);
   setParameterMinimum(m_radius, (Float)0.);
   m_mapMode = addIntParameter("map_mode", "mapping of data to sphere size", (Integer)Fixed, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_mapMode, MapMode);
   m_range = addVectorParameter("range", "allowed radius range", ParamVector(0., MaxRad));
   setParameterMinimum(m_range, ParamVector(0., 0.));
}

ToSpheres::~ToSpheres() {

}

template<typename S>
S clamp(S v, S vmin, S vmax) {
   return std::min(std::max(v, vmin), vmax);
}

bool ToSpheres::compute() {

   auto v = expect<Vec<Scalar, 3>>("grid_in");
   if (!v)
      return true;

   const MapMode mode = (MapMode)m_mapMode->getValue();
   auto radius = accept<Vec<Scalar, 1>>("data_in");
   if (mode != Fixed && !radius) {
      sendError("data input required for varying radius");
      return true;
   }

   Spheres::ptr spheres = Spheres::clone<Vec<Scalar, 3>>(v);
   auto r = spheres->r().data();

   auto rad = radius ? radius->x().data() : nullptr;
   const Scalar scale = m_radius->getValue();
   const Scalar rmin = m_range->getValue()[0];
   const Scalar rmax = m_range->getValue()[1];
   for (Index i=0; i<spheres->getNumSpheres(); ++i) {
      switch (mode) {
         case Fixed:
            r[i] = scale;
            break;
         case Radius:
            r[i] = scale * rad[i];
            break;
         case Surface:
            r[i] = scale * sqrt(rad[i]);
            break;
         case Volume:
            r[i] = scale * powf(rad[i], 0.333333f);
            break;
      }

      r[i] = clamp(r[i], rmin, rmax);
   }
   addObject("grid_out", spheres);

   return true;
}
