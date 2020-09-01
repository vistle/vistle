#include "Exeption.h"
#include <sstream>
#include <mpi.h>
using namespace vistle::insitu;



const char* SimV2Exeption::what() const noexcept {
    m_msg =  "LibSim error: " + m_msg + " a call to a simV2 runtime function failed!";
    return m_msg.c_str();
}

EngineExeption::EngineExeption(const std::string& message) {
        m_msg += message;
    }

const char* EngineExeption::what() const noexcept {

    m_msg = "Engine error: " + m_msg;
    return m_msg.c_str();
}
