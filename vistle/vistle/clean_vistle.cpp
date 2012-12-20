/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */


#include <iostream>
#include <exception>

#include <mpi.h>

#include <core/shm.h>

int main(int argc, char ** argv) {

   int ret = 0;
   try {

      MPI_Init(&argc, &argv);

      ret = vistle::Shm::cleanAll() ? 0 : 1;

      MPI_Finalize();
   } catch(std::exception &e) {

      std::cerr << "fatal exception: " << e.what() << std::endl;
      exit(1);
   }

   return ret;
}
