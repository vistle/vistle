#include "Exeption.h"
#include <sstream>
#include <mpi.h>
using namespace in_situ;


in_situ::VistleLibSimExeption::VistleLibSimExeption() {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    m_msg += "[" + std::to_string(rank) + "/" + std::to_string(size) + "] ";
}

VistleLibSimExeption& in_situ::VistleLibSimExeption::operator<<(const std::string& msg) {
        m_msg += msg;
        return *this;
    }

VistleLibSimExeption& in_situ::VistleLibSimExeption::operator<<(int msg) {
    m_msg += std::to_string(msg);
    return *this;
}

const char* in_situ::SimV2Exeption::what() const {
    return ("LibSim error: " + m_msg + " a call to a simV2 runtime function failed!").c_str();
}

in_situ::EngineExeption::EngineExeption(const std::string& message)
    {
    m_msg += message;
}
const char* in_situ::EngineExeption::what() const {
    return ("Engine error: " + m_msg).c_str();
}


