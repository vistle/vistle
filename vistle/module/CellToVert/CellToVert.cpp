#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/triangles.h>
#include <core/unstr.h>
#include <core/vec.h>

#include "CellToVert.h"
#include "coCellToVert.h"

using namespace vistle;

using namespace vistle;

MODULE_MAIN(CellToVert)

CellToVert::CellToVert(const std::string &shmname, const std::string &name, int moduleID)
   : Module("CellToVert", shmname, name, moduleID) {

   createInputPort("grid_in");
   createInputPort("data_in");

   createOutputPort("grid_out");
   createOutputPort("data_out");
}

CellToVert::~CellToVert() {
}

bool CellToVert::compute() {

   coCellToVert algo;

   while (hasObject("grid_in") && hasObject("data_in")) {

      Object::const_ptr grid = takeFirstObject("grid_in");
      Object::const_ptr data = takeFirstObject("data_in");

      Object::ptr out = algo.interpolate(grid, data);

      if (out) {
         out->copyAttributes(data);
         addObject("data_out", out);
         passThroughObject("grid_out", grid);
      }
   }

   return true;
}
