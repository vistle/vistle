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

   auto v = expect<Vec<Scalar, 3>>("grid_in");
   if (!v) {
      return false;
   }

   Points::ptr points = Points::clone<Vec<Scalar, 3>>(v);
   addObject("grid_out", points);

   return true;
}
