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

   auto d = accept<DataBase>("grid_in");
   Coords::const_ptr grid;
   if (d) {
      grid = Coords::as(d->grid());
   }
   if (!grid) {
      grid = accept<Coords>("grid_in");
   }
   if (!grid) {
      sendError("no grid");
      return true;
   }

   Points::ptr points = Points::clone<Vec<Scalar, 3>>(grid);
   addObject("grid_out", points);

   return true;
}
