#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/geometry.h>

#include "Collect.h"

MODULE_MAIN(Collect)

using namespace vistle;


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

   while (hasObject("grid_in")) {

      if (isConnected("normal_in") && !hasObject("normal_in"))
         break;
      if (isConnected("texture_in") && !hasObject("texture_in"))
         break;
      
      vistle::Object::const_ptr grid = takeFirstObject("grid_in");
      vistle::Geometry::ptr geom(new vistle::Geometry(grid));
      geom->setMeta(grid->meta());

      vistle::Object::const_ptr norm = takeFirstObject("normal_in");
      if (norm)
         geom->setNormals(norm);

      vistle::Object::const_ptr tex = takeFirstObject("texture_in");
      if (tex)
         geom->setTexture(tex);

      addObject("grid_out", geom);
   }

   return true;
}
