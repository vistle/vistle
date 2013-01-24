#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/geometry.h>

#include "Collect.h"

MODULE_MAIN(Collect)


Collect::Collect(const std::string &shmname, int rank, int size, int moduleID)
   : Module("Collect", shmname, rank, size, moduleID) {

   createInputPort("grid_in");
   createInputPort("texture_in");
   createInputPort("normal_in");

   createOutputPort("grid_out");
}

Collect::~Collect() {

}


bool Collect::compute() {

   ObjectList gridObjects = getObjects("grid_in");
   ObjectList textureObjects = getObjects("texture_in");

   while (hasObject("grid_in") && hasObject("texture_in")) {

      vistle::Object::const_ptr grid = takeFirstObject("grid_in");
      vistle::Object::const_ptr tex = takeFirstObject("texture_in");
      vistle::Geometry::ptr geom(new vistle::Geometry(grid));
      geom->setTexture(tex);

      geom->setBlock(grid->getBlock());
      geom->setTimestep(grid->getTimestep());

      addObject("grid_out", geom);
   }

   return true;
}
