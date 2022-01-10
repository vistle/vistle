#include "EngineInterface.h"
using namespace vistle::insitu::libsim;

std::unique_ptr<vistle::insitu::message::InSituTcp> EngineInterface::m_messageHandler;

void EngineInterface::initialize(boost::mpi::communicator comm)
{
    m_messageHandler = std::make_unique<vistle::insitu::message::InSituTcp>(comm);
}

vistle::insitu::message::InSituTcp *EngineInterface::getHandler()
{
    return m_messageHandler.get();
}

std::unique_ptr<vistle::insitu::message::InSituTcp> EngineInterface::extractHandler()
{
    return std::move(m_messageHandler);
}
