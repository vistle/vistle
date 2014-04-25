/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <mpi.h>

#include <iostream>
#include <fstream>
#include <exception>
#include <cstdlib>
#include <sstream>

#include <util/findself.h>
#include <core/object.h>
#include <control/executor.h>

using namespace vistle;

class Vistle: public Executor {
   public:
   Vistle(int argc, char *argv[]) : Executor(argc, argv) {}
   bool config(int argc, char *argv[]) {

      std::string bindir = getbindir(argc, argv);
      return scanModules(bindir + "/../libexec/module");
   }
};

int main(int argc, char ** argv) {

   try {

      MPI_Init(&argc, &argv);
      vistle::registerTypes();
      Vistle(argc, argv).run();
      MPI_Finalize();
   } catch(vistle::exception &e) {

      std::cerr << "fatal exception: " << e.what() << std::endl << e.where() << std::endl;
      exit(1);
   } catch(std::exception &e) {

      std::cerr << "fatal exception: " << e.what() << std::endl;
      exit(1);
   }
   
   return 0;
}
