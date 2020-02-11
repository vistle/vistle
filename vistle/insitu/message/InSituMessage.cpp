#include "InSituMessage.h"
#include <boost/asio.hpp>
#include <iostream>


using namespace insitu;
using namespace insitu::message;
using namespace vistle::message;


bool insitu::message::InSituTcpMessage::m_initialized = false;
boost::mpi::communicator insitu::message::InSituTcpMessage::m_comm;
std::shared_ptr< boost::asio::ip::tcp::socket> insitu::message::InSituTcpMessage::m_socket;


bool insitu::message::InSituShmMessage::m_initialized = false;
std::unique_ptr<vistle::message::MessageQueue> insitu::message::InSituShmMessage::m_sendMessageQueue = nullptr;
std::unique_ptr<vistle::message::MessageQueue> insitu::message::InSituShmMessage::m_receiveMessageQueue = nullptr;
int  insitu::message::InSituShmMessage::m_rank = -1;
int  insitu::message::InSituShmMessage::m_moduleID = -1;

InSituMessageType insitu::message::InSituMessageBase::type() const {
    return m_type;
}

InSituMessageType InSituTcpMessage::type() const{
    return m_type;
}

InSituTcpMessage::InSituTcpMessage(InSituMessageType type, vistle::buffer&& payload)
    :m_type(type)
    ,m_payload(payload){
}

InSituTcpMessage::InSituTcpMessage()
    :m_type(InSituMessageType::Invalid) {

}

InSituTcpMessage InSituTcpMessage::recv() {
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
            vistle::message::recv(*m_socket, bf, err, true, &payload);
            if (err || bf.type() != vistle::message::Type::INSITU) {
                type = static_cast<int>(InSituMessageType::ConnectionClosed);
            } else {
                type = static_cast<int>(bf.as<InSituMessage>().ismType());
            }
        }
    }
    m_comm.barrier();
    boost::mpi::broadcast(m_comm, error, 0);
    if (error) {
        return InSituTcpMessage{};
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
    return InSituTcpMessage{static_cast<InSituMessageType>(type), std::move(payload)};
}

void InSituTcpMessage::initialize(std::shared_ptr<boost::asio::ip::tcp::socket> socket, boost::mpi::communicator comm) {
    m_socket = socket;
    m_comm = comm;
    m_initialized = true;
}

bool InSituTcpMessage::isInitialized() {
    return m_initialized;
}

InSituMessageType InSituShmMessage::type() const {
    return m_type;
}

void InSituShmMessage::initialize(int moduleID, int rank, Mode mode) {

    m_moduleID = moduleID;
    m_rank = rank;

    std::function<vistle::message::MessageQueue * (const std::string&)> f;
    std::string smqName, rmqName;
    switch (mode) {
    case InSituShmMessage::Mode::Create:
        f = vistle::message::MessageQueue::create;
        smqName = vistle::message::MessageQueue::createName("sendInSitu", moduleID, rank);
        rmqName = vistle::message::MessageQueue::createName("recvInSitu", moduleID, rank);
        break;
    case InSituShmMessage::Mode::Attach:
        f = vistle::message::MessageQueue::open;
        smqName = vistle::message::MessageQueue::createName("recvInSitu", moduleID, rank);
        rmqName = vistle::message::MessageQueue::createName("sendInSitu", moduleID, rank);
        break;
    default:
        break;
    }

    try {
        m_sendMessageQueue.reset(f(smqName));
        std::cerr << "sendMessageQueue name = " << smqName << std::endl;
    } catch (boost::interprocess::interprocess_exception & ex) {
        throw vistle::exception(std::string("opening send message queue ") + smqName + ": " + ex.what());
    }

    try {
        m_receiveMessageQueue.reset(f(rmqName));
    } catch (boost::interprocess::interprocess_exception & ex) {
        throw vistle::exception(std::string("opening receive message queue ") + rmqName + ": " + ex.what());
    }

    m_initialized = true;
}

bool InSituShmMessage::isInitialized() {
    return m_initialized;
}

InSituShmMessage InSituShmMessage::recv(bool block) {
    vistle::message::Buffer buf;
    if (block) {
        m_receiveMessageQueue->receive(buf);
    } else if (!m_receiveMessageQueue->tryReceive(buf)) {
        return InSituShmMessage{};
    } 
    if (buf.type() != vistle::message::Type::INSITU) {
        return InSituShmMessage{};
    }
    InSituMessage ism = buf.as<InSituMessage>();
    return InSituShmMessage { ism };
}

InSituShmMessage::InSituShmMessage(const InSituMessage& msg)
    : m_type(msg.ismType())
    , m_payload(msg.payloadName())
    , m_payloadSize(msg.payloadSize()){
}

InSituShmMessage::InSituShmMessage()
    : m_type(InSituMessageType::Invalid) {
}


