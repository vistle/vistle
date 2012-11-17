/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <mpi.h>

#include <iostream>
#include <fstream>

#include <control/executor.h>

using namespace vistle;

class Vistle: public Executor {
   public:
   Vistle(int argc, char *argv[]) : Executor(argc, argv) {}
   bool config(int argc, char *argv[]) {

      if (argc > 1)
         setFile(argv[1]);

      return true;
   }
};

int main(int argc, char ** argv) {

   MPI_Init(&argc, &argv);

   Vistle(argc, argv).run();

   MPI_Finalize();
   
   return 0;
}
