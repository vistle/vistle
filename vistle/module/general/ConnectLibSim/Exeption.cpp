#include "Exeption.h"
#include <sstream>
#include <mpi.h>
using namespace insitu;


insitu::VistleLibSimExeption::VistleLibSimExeption() {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    m_msg += "[" + std::to_string(rank) + "/" + std::to_string(size) + "] ";
}

VistleLibSimExeption& insitu::VistleLibSimExeption::operator<<(const std::string& msg) {
        m_msg += msg;
        return *this;
    }

VistleLibSimExeption& insitu::VistleLibSimExeption::operator<<(int msg) {
    m_msg += std::to_string(msg);
    return *this;
}

const char* insitu::SimV2Exeption::what() const noexcept {
    return ("LibSim error: " + m_msg + " a call to a simV2 runtime function failed!").c_str();
}

insitu::EngineExeption::EngineExeption(const std::string& message)
    {
    m_msg += message;
}
const char* insitu::EngineExeption::what() const noexcept {
    return ("Engine error: " + m_msg).c_str();
}


