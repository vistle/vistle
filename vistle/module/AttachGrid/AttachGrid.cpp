#include <core/object.h>
#include <core/vec.h>
#include <core/coords.h>

#include "AttachGrid.h"

using namespace vistle;

MODULE_MAIN(AttachGrid)

AttachGrid::AttachGrid(const std::string &shmname, const std::string &name, int moduleID)
   : Module("AttachGrid", shmname, name, moduleID) {

   m_gridIn = createInputPort("grid_in");
   m_dataIn = createInputPort("data_in");

   m_dataOut = createOutputPort("data_out");
}

AttachGrid::~AttachGrid() {
}

bool AttachGrid::compute() {

   auto grid = expect<Object>(m_gridIn);
   auto data = expect<DataBase>(m_dataIn);
   if (!grid || !data)
      return true;

   auto out = data->clone();
   out->setGrid(grid);
   addObject(m_dataOut, out);

   return true;
}
