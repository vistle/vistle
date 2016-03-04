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

   createInputPort("data_in");

   createOutputPort("data_out");
}

CellToVert::~CellToVert() {
}

bool CellToVert::compute() {

   coCellToVert algo;

   auto data = expect<DataBase>("data_in");
   if (!data)
      return true;

   auto grid = data->grid();
   if (!grid) {
      sendError("grid is required on input object");
      return true;
   }

   if (data->mapping() == DataBase::Unspecified) {
   } else if (data->mapping() != DataBase::Element) {
      std::stringstream str;
      str << "unsupported data mapping " << data->mapping() << " on " << data->getName();
      std::string s = str.str();
      sendError("%s", s.c_str());
      return true;
   }

   DataBase::ptr out = algo.interpolate(grid, data);
   if (out) {
      out->copyAttributes(data);
      out->setGrid(grid);
      out->setMapping(DataBase::Vertex);
      addObject("data_out", out);
   }

   return true;
}
