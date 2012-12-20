/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <mpi.h>

#include <iostream>
#include <fstream>
#include <exception>

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

   try {

      MPI_Init(&argc, &argv);
      Vistle(argc, argv).run();
      MPI_Finalize();
   } catch(std::exception &e) {

      std::cerr << "fatal exception: " << e.what() << std::endl;
      exit(1);
   }
   
   return 0;
}
