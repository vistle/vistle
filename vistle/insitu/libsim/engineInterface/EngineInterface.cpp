#include "EngineInterface.h"
using namespace insitu;

std::shared_ptr<boost::asio::ip::tcp::socket> EngineInterface::m_socket{};
void EngineInterface::setControllSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    m_socket = socket;
}
std::shared_ptr<boost::asio::ip::tcp::socket> EngineInterface::getControllSocket()
{
    return m_socket;
}
