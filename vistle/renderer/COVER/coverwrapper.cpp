/* wrapper for COVER to distribute environment variables to slaves before
 * initialization of static OpenSceneGraph objects happens */

#include "OsgRenderer.h"


#include <mpi.h>

#if 0
#ifdef MODULE_THREAD
static std::shared_ptr<vistle::Module> newModuleInstance(const std::string &name, int moduleId) {
    int argc=0;
    char *argv[] = { nullptr };
    std::string bindir = vistle::getbindir(argc, argv);
    std::string abslib = setupEnvAndGetLibDir(bindir) + libcover;
       //return std::shared_ptr<X>(new X("dummy shm", name, moduleId));
    return nullptr;
}
BOOST_DLL_ALIAS(newModuleInstance, newModule);
#else
int main(int argc, char *argv[]) {

   int provided = MPI_THREAD_SINGLE;
   MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
   if (provided == MPI_THREAD_SINGLE) {
      std::cerr << "no thread support in MPI" << std::endl;
      exit(1);
   }

   int ret = runMain(argc, argv);

   MPI_Finalize();

   return ret;
}
#endif
#endif

MODULE_MAIN(COVER)
