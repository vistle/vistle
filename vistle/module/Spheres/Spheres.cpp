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

   while (Object::const_ptr obj = takeFirstObject("grid_in")) {

      auto v = Vec<Scalar, 3>::as(obj);
      if (!v) {
         std::cerr << "Spheres: incompatible input" << std::endl;
         continue;
      }

      Spheres::ptr spheres = Spheres::clone<Vec<Scalar, 3>>(v);

      const Scalar radius = m_radius->getValue();
      for (auto &r: spheres->r()) {
         r = radius;
      }
      addObject("grid_out", spheres);
   }

   return true;
}
