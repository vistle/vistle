#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/points.h>

#include "ToPoints.h"

MODULE_MAIN(ToPoints)

using namespace vistle;

ToPoints::ToPoints(const std::string &shmname, const std::string &name, int moduleID)
   : Module("ToPoints", shmname, name, moduleID) {

   createInputPort("grid_in");
   createOutputPort("grid_out");
}

ToPoints::~ToPoints() {

}

bool ToPoints::compute() {

   while (Object::const_ptr obj = takeFirstObject("grid_in")) {

      auto v = Vec<Scalar, 3>::as(obj);
      if (!v) {
         std::cerr << "ToPoints: incompatible input" << std::endl;
         continue;
      }

      Points::ptr points = Points::clone<Vec<Scalar, 3>>(v);
      addObject("grid_out", points);
   }

   return true;
}
