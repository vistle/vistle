#include <sstream>
#include <iomanip>

#include "object.h"
#include "message.h"
#include "messagequeue.h"
#include "gendat.h"

MODULE_MAIN(Gendat)

Gendat::Gendat(int rank, int size, int moduleID)
   : Module("Gendat", rank, size, moduleID) {

   createOutputPort("data");
}

Gendat::~Gendat() {

}

bool Gendat::compute() {

   std::stringstream name;
   name << "Object_" << std::setw(8) << std::setfill('0') << moduleID
        << "_" << std::setw(8) << std::setfill('0') << rank;

   try {
      vistle::FloatArray a(name.str());

      vistle::message::NewObject n(name.str());
      sendMessageQueue->getMessageQueue().send(&n, sizeof(n), 0);

      std::cout << "module " << moduleID << " [" << rank << "/" << size << "]"
         " object [" << name.str() << "] allocated" << std::endl;

      for (int index = 0; index < 128; index ++)
         a.vec->push_back(index);
   } catch (interprocess_exception &ex) {
      std::cout << "module " << moduleID << " [" << rank << "/" << size << "]:"
         " object [" << name.str() << "] already exists" << std::endl;
   }

   int *local = new int[1024 * 1024];
   int *global = new int[1024 * 1024];

   for (int count = 0; count < 128; count ++) {
      for (int index = 0; index < 1024 * 1024; index ++)
         local[index] = rand();
      MPI_Allreduce(local, global, 1024 * 1024, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   }

   return true;
}
