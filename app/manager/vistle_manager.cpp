/*
 * Vistle Parallel Scientific Visualization
 *
 * Started by Florian Niebling in early 2012 as
 * "Visualization Testing Laboratory for Exascale Computing (VISTLE)"
 */

#include <mpi.h>
#include <vistle/manager/manager.h>

#include <iostream>
#include <vistle/util/directory.h>


int main(int argc, char *argv[])
{
    vistle::Directory dir(argc, argv);
    vistle::directory::setVistleRoot(dir.prefix(), dir.buildType());
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
