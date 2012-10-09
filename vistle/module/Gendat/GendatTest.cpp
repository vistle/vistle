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

   enum { GENDAT = 1, CHECKER };

   spawn(GENDAT, "Gendat");
   spawn(CHECKER, "GendatChecker");

   connect(GENDAT, "data_out", CHECKER, "data_in");

   compute(GENDAT);
}
