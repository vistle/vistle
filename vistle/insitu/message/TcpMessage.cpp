#include "TcpMessage.h"
#include <vistle/insitu/core/slowMpi.h>
#include <boost/asio.hpp>
using namespace vistle::insitu;
using namespace vistle::insitu::message;

void InSituTcp::initialize(std::shared_ptr<boost::asio::ip::tcp::socket> socket, boost::mpi::communicator comm)
{
    m_socket = &(*socket);
    m_comm = comm;
    m_initialized = true;
}

bool InSituTcp::isInitialized()
{
    return m_initialized;
}

Message InSituTcp::recv()
{
    vistle::buffer payload;
    bool error = false;

    int type;
    if (!m_initialized) {
        error = true;
    } else {
        if (m_comm.rank() == 0) {
            boost::system::error_code err;
            vistle::message::Buffer bf;
            if (!vistle::message::recv(*m_socket, bf, err, false, &payload)) {
                error = true;
            } else if (err || static_cast<vistle::message::Message &>(bf).type() != vistle::message::Type::INSITU) {
                type = static_cast<int>(InSituMessageType::ConnectionClosed);
                ConnectionClosed proxy{false};
                vistle::vecostreambuf<vistle::buffer> buf;
                vistle::oarchive ar(buf);
                ar &proxy;
                payload = buf.get_vector();
            } else {
                type = static_cast<int>(bf.as<InSituMessage>().ismType());
            }
        }
    }
    vistle::insitu::broadcast(m_comm, error, 0);
    if (error) {
        return Message::errorMessage();
    }
    boost::mpi::broadcast(m_comm, type, 0);
    if (type == static_cast<int>(InSituMessageType::ConnectionClosed)) {
        m_initialized = false;
        m_socket = nullptr;
    }
    int size = payload.size();
    boost::mpi::broadcast(m_comm, size, 0);
    if (m_comm.rank() != 0) {
        payload.resize(size);
    }
    boost::mpi::broadcast(m_comm, payload.data(), payload.size(), 0);
    return Message{static_cast<InSituMessageType>(type), std::move(payload)};
}
