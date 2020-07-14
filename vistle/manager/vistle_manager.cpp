/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <mpi.h>
#include "manager.h"

#include <iostream>
//#include <fstream>
//#include <exception>
//#include <cstdlib>
//#include <sstream>
//#include "run_on_main_thread.h"
//#include <vistle/util/directory.h>
//#include <vistle/core/objectmeta.h>
//#include <vistle/core/object.h>
//
//#include <vistle/util/hostname.h>
//#include <vistle/control/hub.h>
//#include <boost/mpi.hpp>
#include <vistle/util/directory.h>


int main(int argc, char *argv[])
{
    std::string vr = "VISTLE_ROOT=" + vistle::directory::prefix(argc, argv);
    //std::cerr << "setting VISTLE_ROOT to " << vr << std::endl;
    char* vistleRoot = strdup(vr.c_str());
    int retval = putenv(vistleRoot);
#ifdef MODULE_THREAD
   int provided = MPI_THREAD_SINGLE;
   MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
   if (provided != MPI_THREAD_MULTIPLE) {
      std::cerr << "insufficient thread support in MPI: MPI_THREAD_MULTIPLE is required (maybe set MPICH_MAX_THREAD_SAFETY=multiple?)" << std::endl;
      exit(1);
   }
#else
   MPI_Init(&argc, &argv);
#endif

   {
       // ensure that manager is destroyed before MPI_Finalize
       vistle::VistleManager manager;
       manager.run(argc, argv);
   }

   MPI_Finalize();
   return 0;
}
