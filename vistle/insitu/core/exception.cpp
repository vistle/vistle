#include "exception.h"
#include <mpi.h>
#include <sstream>
using namespace vistle::insitu;

InsituException::InsituException()
{
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    m_msg += "[" + std::to_string(rank) + "/" + std::to_string(size) + "] ";
}

InsituException &InsituException::operator<<(const std::string &msg)
{
    m_msg += msg;
    return *this;
}

InsituException &InsituException::operator<<(int msg)
{
    m_msg += std::to_string(msg);
    return *this;
}
