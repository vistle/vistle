#include "exeption.h"
#include <mpi.h>
#include <sstream>
using namespace vistle::insitu;

InsituExeption::InsituExeption()
{
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    m_msg += "[" + std::to_string(rank) + "/" + std::to_string(size) + "] ";
}

InsituExeption &InsituExeption::operator<<(const std::string &msg)
{
    m_msg += msg;
    return *this;
}

InsituExeption &InsituExeption::operator<<(int msg)
{
    m_msg += std::to_string(msg);
    return *this;
}
