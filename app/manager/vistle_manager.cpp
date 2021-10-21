/*
 * Vistle Parallel Scientific Visualization
 *
 * Started by Florian Niebling in early 2012 as
 * "Visualization Testing Laboratory for Exascale Computing (VISTLE)"
 */

#include <mpi.h>
#include "manager.h"

#include <iostream>
#include <vistle/util/directory.h>


int main(int argc, char *argv[])
{
    std::string vr = "VISTLE_ROOT=" + vistle::directory::prefix(argc, argv);
    //std::cerr << "setting VISTLE_ROOT to " << vr << std::endl;
    char *vistleRoot = strdup(vr.c_str());
    int retval = putenv(vistleRoot);
    if (retval != 0) {
        std::cerr << "failed to set VISTLE_ROOT: " << strerror(errno) << std::endl;
    }
#ifdef MODULE_THREAD
    int provided = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        std::cerr << "insufficient thread support in MPI: MPI_THREAD_MULTIPLE is required (maybe set "
                     "MPICH_MAX_THREAD_SAFETY=multiple?)"
                  << std::endl;
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
