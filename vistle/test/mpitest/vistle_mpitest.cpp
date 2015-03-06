#include <mpi.h>
#include <iostream>
#include <sstream>

#ifdef __linux__
#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#define AFFINITY
#endif

int main(int argc, char *argv[]) {

   std::cerr << "trying MPI_Init..." << std::endl;
   MPI_Init(&argc, &argv);
   std::cerr << "after MPI_Init" << std::endl;

   int rank=-1, size=-1;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);

   std::cerr << "rank " << rank << "/" << size << std::endl;
#ifdef AFFINITY
   const int ncpu = 1024;
   cpu_set_t *mask = CPU_ALLOC(ncpu);
   if (!mask) {
      std::cerr << "CPU_ALLOC failed" << std::endl;
   }  else {
      size_t size = CPU_ALLOC_SIZE(ncpu);
      std::cerr << "cpu size " << size << std::endl;
      CPU_ZERO_S(size, mask);
      if(sched_getaffinity(getpid(), size, mask) == -1) {
         perror("sched_getaffinity");
      } else {
         std::stringstream str;
         str << "affinity mask on rank " << rank << ": ";
         for (int i=0; i<16; ++i) {
            str << (CPU_ISSET_S(i, size, mask) ? "1" : "0");
         }
         std::cerr << str.str() << std::endl;
      }
      CPU_FREE(mask);
   }
#endif

   std::cerr << "trying MPI_Finalize..." << std::endl;
   MPI_Finalize();
   std::cerr << "after MPI_Finalize" << std::endl;

   return 0;
}
