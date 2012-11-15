#include <mpi.h>

#include <iostream>

#include <control/executor.h>

class Vistle: public vistle::Executor {

   public:
   Vistle(int argc, char *argv[]) : Executor(argc, argv) {}
   void config();
};

int main(int argc, char ** argv) {

   MPI_Init(&argc, &argv);

   Vistle(argc, argv).run();

   MPI_Finalize();
   
   return 0;
}

void Vistle::config() {

   setInput(
         "gendat = spawn('Gendat');"
         "checker = spawn('GendatChecker');"
         "connect(gendat, 'data_out', checker, 'data_in');"
         "compute(gendat);"
      );
}
