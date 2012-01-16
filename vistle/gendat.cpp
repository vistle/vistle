#include <sstream>
#include <iomanip>

#include "object.h"
#include "message.h"
#include "messagequeue.h"
#include "gendat.h"

MODULE_MAIN(Gendat)

Gendat::Gendat(int rank, int size, int moduleID)
   : Module("Gendat", rank, size, moduleID) {

   createOutputPort("data_out");
}

Gendat::~Gendat() {

}

bool Gendat::compute() {

   std::string name = vistle::Shm::instance().createObjectID();

   try {
      vistle::FloatArray a(name);
      for (int index = 0; index < 128; index ++)
         a.vec->push_back(index);
   } catch (interprocess_exception &ex) {
      std::cout << "module " << moduleID << " [" << rank << "/" << size << "]:"
         " object already exists" << std::endl;
   }

   addObject("data" /* port */, name);

#if 0

   int *local = new int[1024 * 1024];
   int *global = new int[1024 * 1024];

   for (int count = 0; count < 128; count ++) {
      for (int index = 0; index < 1024 * 1024; index ++)
         local[index] = rand();
      MPI_Allreduce(local, global, 1024 * 1024, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   }
#endif
   return true;
}
