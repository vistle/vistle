#include <mpi.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {

   int rank, size;
   MPI_Init(&argc, &argv);

   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);

   printf("  module %d [%02d/%02d]\n", getpid(), rank, size);
   
   int *local = new int[1024 * 64];
   int *global = new int[1024 * 64];

   for (int count = 0; count < 1024; count ++) {
      for (int index = 0; index < 1024 * 64; index ++)
         local[index] = rand(); 
      MPI_Allreduce(local, global, 1024 * 64, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   }
   
   MPI_Finalize();
   printf("  module %d [%02d/%02d] done\n", getpid(), rank, size);

   return 0;
}
