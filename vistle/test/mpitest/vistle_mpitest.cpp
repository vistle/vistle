#include <mpi.h>
#include <iostream>
#include <sstream>

#include <util/affinity.h>
#include <util/hostname.h>

int main(int argc, char *argv[]) {

   //std::cerr << "trying MPI_Init..." << std::endl;
   int provided = MPI_THREAD_SINGLE;
   MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
   //std::cerr << "after MPI_Init" << std::endl;

   int rank=-1, size=-1;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);

   for (int i=0; i<size; ++i) {
       MPI_Barrier(MPI_COMM_WORLD);
       if (i == rank) {
           std::cerr << "rank " << rank << "/" << size << " on host " << vistle::hostname() << ", process affinity: " << vistle::sched_affinity_map() << std::endl;
       }
   }
   MPI_Barrier(MPI_COMM_WORLD);

   std::cerr << "trying MPI_Finalize..." << std::endl;
   MPI_Finalize();
   std::cerr << "after MPI_Finalize" << std::endl;

   return 0;
}
