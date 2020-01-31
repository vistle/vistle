#include "EngineMessage.h"
#include <boost/asio.hpp>
#include <iostream>

using namespace in_situ;
using namespace vistle::message;


bool in_situ::EngineMessage::m_initialized = false;
boost::mpi::communicator in_situ::EngineMessage::m_comm;
std::shared_ptr< boost::asio::ip::tcp::socket> in_situ::EngineMessage::m_socket;

EngineMessageType EngineMessage::type() const{
    return m_type;
}

EngineMessage::EngineMessage(EngineMessageType type, vistle::buffer&& payload)
    :m_type(type)
    ,m_payload(payload)
    , m_msg(EngineMessageType::Invalid) {
    
}

EngineMessage::EngineMessage()
    :m_type(EngineMessageType::Invalid)
    , m_msg(EngineMessageType::Invalid) {

}



EngineMessage EngineMessage::recvEngineMessage() {
    bool error = false;
    vistle::buffer payload;
    int type;
    if (!m_initialized) {
        error = true;
    }
    else {
        if (m_comm.rank() == 0) {
            boost::system::error_code err;
            vistle::message::Buffer bf;
            vistle::message::recv(*m_socket, bf, err, false, &payload);
            if (err || bf.type() != vistle::message::Type::INSITU) {
                error = true;
            } else {
                type = static_cast<int>(bf.as<InSituMessage>().emType);
            }
        }
    }

    boost::mpi::broadcast(m_comm, error, 0);
    if (error) {
        return EngineMessage{};
    }
    boost::mpi::broadcast(m_comm, type, 0);
    boost::mpi::broadcast(m_comm, payload.data(), payload.size(), 0);
    return EngineMessage{static_cast<EngineMessageType>(type), std::move(payload)};
}

void EngineMessage::initializeEngineMessage(std::shared_ptr<boost::asio::ip::tcp::socket> socket, boost::mpi::communicator comm) {
    m_socket = socket;
    m_comm = comm;
    m_initialized = true;
}

bool EngineMessage::isInitialized() {
    return m_initialized;
}
