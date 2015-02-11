#include <mpi.h>
#include <iostream>

int main(int argc, char *argv[]) {

   std::cerr << "trying MPI_Init..." << std::endl;
   MPI_Init(&argc, &argv);
   std::cerr << "after MPI_Init" << std::endl;

   int rank=-1, size=-1;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);

   std::cerr << "rank " << rank << "/" << size << std::endl;

   std::cerr << "trying MPI_Finalize..." << std::endl;
   MPI_Finalize();
   std::cerr << "after MPI_Finalize" << std::endl;

   return 0;
}
