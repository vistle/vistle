#include <memory>

#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>

#include <iostream>
#include <string>

#include <mpi.h>


template<typename T>
static T min(T a, T b)
{
    return a < b ? a : b;
}

using namespace boost::interprocess;

int main(int argc, char *argv[])
{
    int shift = 30;
    if (argc > 1)
        shift = atoi(argv[1]);

    MPI_Init(&argc, &argv);

    shared_memory_object::remove("test");

    managed_shared_memory *shm = NULL;
    try {
        shm = new managed_shared_memory(create_only, "/test", 1LL << shift);
    } catch (std::exception &ex) {
        std::cerr << "exception: " << ex.what() << std::endl;
        MPI_Finalize();
        exit(1);
    }

    if (shm) {
        std::cerr << "success" << std::endl;
    } else {
        std::cerr << "failure" << std::endl;
    }

    MPI_Finalize();

    return 0;
}
