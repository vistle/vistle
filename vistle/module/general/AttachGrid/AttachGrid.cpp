#include <core/object.h>
#include <core/vec.h>
#include <core/coords.h>

#include "AttachGrid.h"

using namespace vistle;

MODULE_MAIN(AttachGrid)

AttachGrid::AttachGrid(const std::string &shmname, const std::string &name, int moduleID)
   : Module("AttachGrid", shmname, name, moduleID) {

   m_gridIn = createInputPort("grid_in");

   for (int i=0; i<5; ++i) {
      std::string number = boost::lexical_cast<std::string>(i);
      m_dataIn.push_back(createInputPort("data_in"+number));
      m_dataOut.push_back(createOutputPort("data_out"+number));
   }
}

AttachGrid::~AttachGrid() {
}

bool AttachGrid::compute() {

   auto grid = expect<Object>(m_gridIn);
   if (!grid)
       return true;

   for (size_t i=0; i<m_dataIn.size(); ++i) {

       auto &pin = m_dataIn[i];
       auto &pout = m_dataOut[i];
       if (isConnected(pin)) {
          auto data = expect<DataBase>(pin);
          if (!data) {
              return true;
          }
          auto out = data->clone();
          out->setGrid(grid);
          addObject(pout, out);
       }
   }

   return true;
}
