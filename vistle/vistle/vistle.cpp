/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <mpi.h>

#include <iostream>
#include <fstream>
#include <exception>
#include <cstdlib>
#include <sstream>

#include <control/executor.h>
#include <util/findself.h>

using namespace vistle;

class Vistle: public Executor {
   public:
   Vistle(int argc, char *argv[]) : Executor(argc, argv) {}
   bool config(int argc, char *argv[]) {

      if (argc > 1)
         setFile(argv[1]);

      std::string bindir = getbindir(argc, argv);

      int rank = 0;
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);

      if (rank == 0) {
         const pid_t pid = fork();
         if (pid < 0) {
            std::cerr << "Error when forking for executing GUI: " << strerror(errno) << std::endl;
            exit(1);
         } else if (pid == 0) {
            std::string gui = bindir + "/gui";
            std::stringstream s;
            s << uiPort();
            execlp(gui.c_str(), "gui", "localhost", s.str().c_str(), nullptr);
            std::cerr << "Error when executing " << gui << ": " << strerror(errno) << std::endl;
            exit(1);
         }
      }

      return true;
   }
};

int main(int argc, char ** argv) {

   try {

      MPI_Init(&argc, &argv);
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
