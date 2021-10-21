/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */


#include <iostream>
#include <exception>

#include <mpi.h>

#include <vistle/core/shm.h>
#include <vistle/util/shmconfig.h>

int main(int argc, char **argv)
{
    int ret = 0;
    try {
        MPI_Init(&argc, &argv);
        int rank = 0;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        bool perRank = vistle::shmPerRank();

        if (argc == 2) {
            vistle::Shm::remove(argv[1], 0, rank, perRank);
        } else {
            vistle::Shm::cleanAll(rank);
        }

        MPI_Finalize();
    } catch (std::exception &e) {
        std::cerr << "fatal exception: " << e.what() << std::endl;
        exit(1);
    }

    return ret;
}
