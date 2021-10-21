#include <mpi.h>
#include <iostream>
#include <sstream>

#include <vistle/util/affinity.h>
#include <vistle/util/hostname.h>

int main(int argc, char *argv[])
{
    bool threaded = true;
    if (argc > 1) {
        auto arg1 = std::string(argv[1]);
        if (arg1 == "-single" || arg1 == "-s")
            threaded = false;
    }

    int provided = MPI_THREAD_SINGLE;
    if (threaded) {
        std::cerr << "MPI_Init_thread..." << std::endl;
        MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    } else {
        std::cerr << "MPI_Init..." << std::endl;
        MPI_Init(&argc, &argv);
    }
    //std::cerr << "after MPI_Init" << std::endl;

    int rank = -1, size = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for (int i = 0; i < size; ++i) {
        MPI_Barrier(MPI_COMM_WORLD);
        if (i == rank) {
            std::cerr << "rank " << rank << "/" << size << " on host " << vistle::hostname()
                      << ", process affinity: " << vistle::sched_affinity_map() << std::endl;
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    int sum = 0;
    MPI_Allreduce(&rank, &sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (rank == size - 1) {
        std::cerr << "rank " << rank << "/" << size << ": sum of all ranks: " << sum << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    std::cerr << "trying MPI_Finalize..." << std::endl;
    MPI_Finalize();
    std::cerr << "after MPI_Finalize" << std::endl;

    return 0;
}
