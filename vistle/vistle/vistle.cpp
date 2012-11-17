/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <mpi.h>

#include <iostream>
#include <fstream>

#include <control/executor.h>
#include <control/pythonembed.h>

using namespace vistle;

class Vistle: public vistle::Executor {

   public:
   Vistle(int argc, char *argv[]) : Executor(argc, argv) {}
   void config();
};

int main(int argc, char ** argv) {

   MPI_Init(&argc, &argv);

   PythonEmbed *pi = NULL;

   {
      Vistle vistle(argc, argv);

      if (!vistle.getRank()) {
         pi = new PythonEmbed(argc, argv);
         vistle.registerInterpreter(pi);
         if (argc > 1) {
            std::ifstream file(argv[1]);
            std::string input;

            input.insert(input.begin(),
                  std::istreambuf_iterator<char>(file),
                  std::istreambuf_iterator<char>());
            std::cerr << "INPUT was: " << input << std::endl;
            vistle.setInput(input);
         }
      }

      vistle.run();
   }

   delete pi;

   MPI_Finalize();
   
   return 0;
}

void Vistle::config() {
}
