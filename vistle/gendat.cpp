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

   vistle::Vec<float> *a = vistle::Vec<float>::create(64);
   for (unsigned int index = 0; index < a->getSize(); index ++)
      a->x[index] = (float) index;

   vistle::Vec3<int> *b = vistle::Vec3<int>::create(64);
   for (unsigned int index = 0; index < b->getSize(); index ++)
      b->x[index] = index;

   addObject("data_out", a);
   addObject("data_out", b);

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
