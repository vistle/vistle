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

      std::string bindir = getbindir(argc, argv);

      bool start_gui = true;
      bool start_tui = false;
      if (argc > 1) {
         bool haveUiArg = true;
         std::string arg = argv[1];
         if (arg == "-no-ui" || arg == "-batch") {
            start_gui = false;
            start_tui = false;
         } else if (arg == "-gui") {
            start_gui = true;
            start_tui = false;
         } else if (arg == "-tui") {
            start_gui = false;
            start_tui = true;
         } else {
            haveUiArg = false;
         }

         if (haveUiArg) {
            --argc;
            ++argv;
         }
      }

      if (argc > 1)
         setFile(argv[1]);

      int rank = 0;
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);

      if (rank == 0 && (start_gui || start_tui)) {
         const pid_t pid = fork();
         if (pid < 0) {
            std::cerr << "Error when forking for executing UI: " << strerror(errno) << std::endl;
            exit(1);
         } else if (pid == 0) {
            std::string cmd = "gui";
            if (start_tui)
               cmd = "blower";
            std::string pathname = bindir + "/" + cmd;
            std::stringstream s;
            s << uiPort();
            execlp(pathname.c_str(), cmd.c_str(), "-from-vistle", "localhost", s.str().c_str(), nullptr);
            std::cerr << "Error when executing " << pathname << ": " << strerror(errno) << std::endl;
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
