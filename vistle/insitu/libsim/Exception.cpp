#include "Exception.h"
#include <mpi.h>
#include <sstream>
using namespace vistle::insitu;

const char *SimV2Exception::what() const noexcept
{
    m_msg = "LibSim error: " + m_msg + " a call to a simV2 runtime function failed!";
    return m_msg.c_str();
}

EngineException::EngineException(const std::string &message)
{
    m_msg += message;
}

const char *EngineException::what() const noexcept
{
    m_msg = "Engine error: " + m_msg;
    return m_msg.c_str();
}
