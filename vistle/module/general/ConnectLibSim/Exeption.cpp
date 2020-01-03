#include "Exeption.h"
#include <sstream>
#include <mpi.h>
using namespace in_situ;

const char* in_situ::SimV2Exeption::what() const throw () {
    int rank = -1, size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    std::stringstream s;
    s << "[" << rank << "/" << size << "] " << "a call to a simV2 runntime function failed!";
    return s.str().c_str();
}

in_situ::EngineExeption::EngineExeption(const std::string& message)
    :msg(message) {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
    MPI_Comm_size(MPI_COMM_WORLD, &size); 
}
const char* in_situ::EngineExeption::what() const throw () {
    std::stringstream s;
    s << "[" << rank << "/" << size << "] " << "Engine error: " << msg;
    return s.str().c_str();
}