/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */


#include <iostream>

#include <mpi.h>

#include <core/shm.h>

int main(int argc, char ** argv) {

   MPI_Init(&argc, &argv);

   int ret = vistle::Shm::cleanAll() ? 0 : 1;

   MPI_Finalize();

   return ret;
}
