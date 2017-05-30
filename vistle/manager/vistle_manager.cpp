/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <mpi.h>

#include <iostream>
#include <fstream>
#include <exception>
#include <cstdlib>
#include <sstream>

#include <util/directory.h>
#include <core/objectmeta.h>
#include <core/object.h>
#include "executor.h"

using namespace vistle;
namespace dir = vistle::directory;

class Vistle: public Executor {
   public:
   Vistle(int argc, char *argv[]) : Executor(argc, argv) {}
   bool config(int argc, char *argv[]) {

      std::string prefix = dir::prefix(argc, argv);
      setModuleDir(dir::module(prefix));
      return true;
   }
};

int main(int argc, char ** argv) {

   try {

      int provided = MPI_THREAD_SINGLE;
      MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
      if (provided == MPI_THREAD_SINGLE) {
         std::cerr << "no thread support in MPI" << std::endl;
         exit(1);
      }

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
