#include <mpi.h>

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <unistd.h>

#include <iostream>

int main(int argc, char **argv) {

   int rank, size, irank, isize;
   MPI_Init(&argc, &argv);

   if (argc != 2) {

      std::cerr << "module missing module ID" << std::endl;
      exit(1);
   }

   int moduleID = atoi(argv[1]);

   MPI_Comm parentCommunicator;
   MPI_Comm_get_parent(&parentCommunicator);
   MPI_Comm_rank(parentCommunicator, &irank);
   MPI_Comm_size(parentCommunicator, &isize);

   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);

   std::cout << "module " << moduleID << " [" << rank << "/" << size
             << "] started" << std::endl;

   int *local = new int[1024 * 1024];
   int *global = new int[1024 * 1024];

   for (int count = 0; count < 256; count ++) {
      for (int index = 0; index < 1024 * 1024; index ++)
         local[index] = rand();
      MPI_Allreduce(local, global, 1024 * 1024, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   }

   std::cout << "module " << moduleID << " [" << rank << "/" << size
             << "] done" << std::endl;

   MPI_Finalize();

   return 0;
}
