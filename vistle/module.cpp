#include <mpi.h>

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <unistd.h>

#include <sstream>
#include <iostream>
#include <iomanip>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "object.h"

using namespace boost::interprocess;

int main(int argc, char **argv) {

   int rank, size, irank, isize;
   MPI_Init(&argc, &argv);

   if (argc != 3) {

      std::cerr << "module missing parameters" << std::endl;
      exit(1);
   }

   MPI_Comm parentCommunicator;
   MPI_Comm_get_parent(&parentCommunicator);
   MPI_Comm_rank(parentCommunicator, &irank);
   MPI_Comm_size(parentCommunicator, &isize);

   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);

   vistle::Shm::instance();

   int moduleID = atoi(argv[1]);

   std::stringstream name;
   name << "Object_" << std::setw(8) << std::setfill('0') << moduleID
        << "_" << std::setw(8) << std::setfill('0') << rank;
   
   try {
      vistle::FloatArray a(name.str());

      std::cout << "module " << moduleID << " [" << rank << "/" << size << "]:"
         " object [" << name.str() << "] allocated" << std::endl; 
      
      for (int index = 0; index < 128; index ++)
         a.vec->push_back(index);
   } catch (interprocess_exception e) {
      std::cout << "module " << moduleID << " [" << rank << "/" << size << "]:"
         " object [" << name.str() << "] already exists" << std::endl; 
   }
   
   std::cout << "module " << moduleID << " [" << rank << "/" << size
             << "] started" << std::endl;

   int *local = new int[1024 * 1024];
   int *global = new int[1024 * 1024];

   for (int count = 0; count < 4; count ++) {
      for (int index = 0; index < 1024 * 1024; index ++)
         local[index] = rand();
      MPI_Allreduce(local, global, 1024 * 1024, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   }

   std::cout << "module " << moduleID << " [" << rank << "/" << size
             << "] done" << std::endl;

   MPI_Finalize();

   return 0;
}
