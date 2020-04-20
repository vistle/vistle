#include "Exeption.h"
#include <sstream>
#include <mpi.h>
using namespace insitu;



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


