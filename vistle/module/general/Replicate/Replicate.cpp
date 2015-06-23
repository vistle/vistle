#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/triangles.h>
#include <core/unstr.h>
#include <core/vec.h>

#include "Replicate.h"

using namespace vistle;

using namespace vistle;

MODULE_MAIN(Replicate)

Replicate::Replicate(const std::string &shmname, const std::string &name, int moduleID)
   : Module("Replicate", shmname, name, moduleID) {

   createInputPort("grid_in");
   createInputPort("data_in");

   createOutputPort("grid_out");
}

Replicate::~Replicate() {
}

bool Replicate::compute() {

   if (hasObject("grid_in")) {
      Object::const_ptr obj = takeFirstObject("grid_in");
      m_objs[obj->getBlock()] = obj;
      //std::cerr << "Replicate: grid " << obj->getName() << std::endl;
   }

   if (!m_objs.empty()) {
      while (hasObject("data_in")) {

         Object::const_ptr data = takeFirstObject("data_in");
#if 0
         std::cerr << "Replicate: data " << data->getName()
            << ", time: " << data->getTimestep() << "/" << data->getNumTimesteps()
            << ", block: " << data->getBlock() << "/" << data->getNumBlocks()
            << std::endl;
#endif
         auto it = m_objs.find(data->getBlock());
         if (it == m_objs.end()) {
            std::cerr << "did not find grid for block " << data->getBlock() << std::endl;
         } else {
            Object::const_ptr grid = m_objs[data->getBlock()];
            passThroughObject("grid_out", grid);
         }

      }
   }

   return true;
}
