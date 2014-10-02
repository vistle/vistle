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

      if (argc == 2) {

         int rank = 0;
         MPI_Comm_rank(MPI_COMM_WORLD, &rank);
         vistle::Shm::attach(argv[1], 0, rank);
         vistle::Shm::the().setRemoveOnDetach();
         vistle::Shm::the().detach();
      } else {

         ret = vistle::Shm::cleanAll() ? 0 : 1;
      }

      MPI_Finalize();
   } catch(std::exception &e) {

      std::cerr << "fatal exception: " << e.what() << std::endl;
      exit(1);
   }

   return ret;
}
