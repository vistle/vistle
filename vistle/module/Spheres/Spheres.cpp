#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/spheres.h>

#include "Spheres.h"

MODULE_MAIN(ToSpheres)

using namespace vistle;

ToSpheres::ToSpheres(const std::string &shmname, const std::string &name, int moduleID)
   : Module("Spheres", shmname, name, moduleID) {

   createInputPort("grid_in");
   createOutputPort("grid_out");

   m_radius = addFloatParameter("radius", "radius of spheres", 1.);
}

ToSpheres::~ToSpheres() {

}

bool ToSpheres::compute() {

   auto v = expect<Vec<Scalar, 3>>("grid_in");
   if (!v)
      return false;

   Spheres::ptr spheres = Spheres::clone<Vec<Scalar, 3>>(v);

   const Scalar radius = m_radius->getValue();
   for (auto &r: spheres->r()) {
      r = radius;
   }
   addObject("grid_out", spheres);

   return true;
}
