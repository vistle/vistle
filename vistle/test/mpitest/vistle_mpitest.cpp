#include <mpi.h>
#include <iostream>

int main(int argc, char *argv[]) {

   std::cerr << "trying MPI_Init..." << std::endl;
   MPI_Init(&argc, &argv);
   std::cerr << "after MPI_Init" << std::endl;

   std::cerr << "trying MPI_Finalize..." << std::endl;
   MPI_Finalize();
   std::cerr << "after MPI_Finalize" << std::endl;

   return 0;
}
