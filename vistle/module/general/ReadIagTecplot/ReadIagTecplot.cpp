#include <boost/algorithm/string/predicate.hpp>

#include <core/object.h>
#include <core/vec.h>
#include <core/polygons.h>
#include <core/lines.h>
#include <core/points.h>
#include <core/unstr.h>
#include <core/uniformgrid.h>
#include <core/rectilineargrid.h>
#include <core/structuredgrid.h>

#include "ReadIagTecplot.h"
#include <util/filesystem.h>

MODULE_MAIN(ReadIagTecplot)

using namespace vistle;

const std::string Invalid("(NONE)");

void ReadIagTecplot::setChoices() {

    std::vector<std::string> fields({Invalid});
    if (m_tecplot) {
        auto append = m_tecplot->Variables();
        std::copy(append.begin(), append.end(), std::back_inserter(fields));
    }

    for (int i=0; i<NumPorts; ++i)
    {
        setParameterChoices(m_cellDataChoice[i], fields);
    }
}


ReadIagTecplot::ReadIagTecplot(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("read IAG Tecplot data", name, moduleID, comm)
{

   createOutputPort("grid_out");
   m_filename = addStringParameter("filename", "name of Tecplot file", "", Parameter::ExistingFilename);
   for (int i=0; i<NumPorts; ++i) {
      std::stringstream spara;
      spara << "cell_field_" << i;
      m_cellDataChoice[i] = addStringParameter(spara.str(), "cell data field", "", Parameter::Choice);
   }


#if 0
   for (int i=0; i<NumPorts; ++i) {
      std::stringstream spara;
      spara << "point_field_" << i;
      m_pointDataChoice[i] = addStringParameter(spara.str(), "point data field", "", Parameter::Choice);

      std::stringstream sport;
      sport << "point_data" << i;
      m_pointPort[i] = createOutputPort(sport.str(), "vertex data");
   }
   for (int i=0; i<NumPorts; ++i) {
      std::stringstream spara;
      spara << "cell_field_" << i;
      m_cellDataChoice[i] = addStringParameter(spara.str(), "cell data field", "", Parameter::Choice);

      std::stringstream sport;
      sport << "cell_data" << i;
      m_cellPort[i] = createOutputPort(sport.str(), "cell data");
   }
#endif
}

ReadIagTecplot::~ReadIagTecplot() {

}

bool ReadIagTecplot::changeParameter(const vistle::Parameter *p) {
   if (p == m_filename) {
      const std::string filename = m_filename->getValue();
      try {
          m_tecplot.reset(new TecplotFile(filename));
      } catch(...) {
          std::cerr << "failed to create TecplotFile for " << filename << std::endl;
      }
      setChoices();
   }

   return Module::changeParameter(p);
}

bool ReadIagTecplot::compute() {

   if (!m_tecplot) {
       if (rank() == 0)
           sendInfo("no Tecplot file open, tried %s", m_filename->getValue().c_str());
       return true;
   }

   size_t numZones = m_tecplot->NumZones();
   std::cerr << "reading " << numZones << " zones" << std::endl;
   for (size_t i=0; i<numZones; ++i) {
       auto mesh = m_tecplot->ReadZone(i);
       delete mesh;
   }

   return true;
}
