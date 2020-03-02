#include "InSituMessage.h"
#include <boost/asio.hpp>
#include <iostream>

#include <boost/interprocess/creation_tags.hpp>
#include <core/messagequeue.h>

using namespace insitu;
using namespace insitu::message;
using namespace vistle::message;


bool insitu::message::InSituTcpMessage::m_initialized = false;
boost::mpi::communicator insitu::message::InSituTcpMessage::m_comm;
std::shared_ptr< boost::asio::ip::tcp::socket> insitu::message::InSituTcpMessage::m_socket;


bool insitu::message::SyncShmMessage::m_initialized = false;
std::unique_ptr<boost::interprocess::message_queue> insitu::message::SyncShmMessage::m_sendMessageQueue = nullptr;
std::unique_ptr<boost::interprocess::message_queue> insitu::message::SyncShmMessage::m_receiveMessageQueue = nullptr;
int  insitu::message::SyncShmMessage::m_rank = -1;
int  insitu::message::SyncShmMessage::m_moduleID = -1;

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
    vistle::buffer payload;
    bool error = false;

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


void SyncShmMessage::initialize(int moduleID, int rank, Mode mode) {

    m_moduleID = moduleID;
    m_rank = rank;

    std::string smqName, rmqName;
    using namespace boost::interprocess;


    switch (mode) {
    case SyncShmMessage::Mode::Create:
        smqName = vistle::message::MessageQueue::createName("sendInSitu", moduleID, rank);
        rmqName = vistle::message::MessageQueue::createName("recvInSitu", moduleID, rank);
        try {
            message_queue::remove(smqName.c_str());
            message_queue::remove(rmqName.c_str());
            m_sendMessageQueue.reset(new message_queue(create_only, smqName.c_str(), 5, sizeof(SyncShmMessage)));
            std::cerr << "sendMessageQueue name = " << smqName << std::endl;
        } catch (boost::interprocess::interprocess_exception & ex) {
            throw vistle::exception(std::string("opening send message queue ") + smqName + ": " + ex.what());
        }

        try {
            m_receiveMessageQueue.reset(new message_queue(create_only, rmqName.c_str(), 5, sizeof(SyncShmMessage)));
            std::cerr << "receiveMessageQueue name = " << rmqName << std::endl;
        } catch (boost::interprocess::interprocess_exception & ex) {
            throw vistle::exception(std::string("opening receive message queue ") + rmqName + ": " + ex.what());
        }

        break;
    case SyncShmMessage::Mode::Attach:
        smqName = vistle::message::MessageQueue::createName("recvInSitu", moduleID, rank);
        rmqName = vistle::message::MessageQueue::createName("sendInSitu", moduleID, rank);
        try {
            m_sendMessageQueue.reset(new message_queue(open_only, smqName.c_str()));
            std::cerr << "sendMessageQueue name = " << smqName << std::endl;
        } catch (boost::interprocess::interprocess_exception & ex) {
            throw vistle::exception(std::string("opening send message queue ") + smqName + ": " + ex.what());
        }

        try {
            m_receiveMessageQueue.reset(new message_queue(open_only, rmqName.c_str()));
            std::cerr << "receiveMessageQueue name = " << rmqName << std::endl;
        } catch (boost::interprocess::interprocess_exception & ex) {
            throw vistle::exception(std::string("opening receive message queue ") + rmqName + ": " + ex.what());
        }
        message_queue::remove(smqName.c_str());
        message_queue::remove(rmqName.c_str());
        break;
    default:
        break;
    }

    m_initialized = true;
}
void SyncShmMessage::close()     {
    m_sendMessageQueue.reset(nullptr);
    m_receiveMessageQueue.reset(nullptr);
    m_initialized = false;
}

bool SyncShmMessage::isInitialized() {
    return m_initialized;
}

bool SyncShmMessage::send(const SyncShmMessage& msg) {
    if (!m_initialized) {
        std::cerr << "SyncShmMessage uninitialized: can not send message!" << std::endl;
        return false;
    }
    m_sendMessageQueue->send(&msg, sizeof(SyncShmMessage), 0);
    return true;
}

SyncShmMessage SyncShmMessage::recv() {
    assert(m_initialized);
    SyncShmMessage msg(0, 0);
    size_t recvSize;
    unsigned int priority;
    m_receiveMessageQueue->receive(&msg, sizeof(SyncShmMessage), recvSize, priority);
    return msg;
}

SyncShmMessage SyncShmMessage::tryRecv(bool& received) {
    assert(m_initialized);
    SyncShmMessage msg(0, 0);
    size_t recvSize;
    unsigned int priority;
    received = m_receiveMessageQueue->try_receive(&msg, sizeof(SyncShmMessage), recvSize, priority);
    return msg;
}

SyncShmMessage SyncShmMessage::timedRecv(int time, bool& received) {
    assert(m_initialized);
    SyncShmMessage msg(0, 0);
    size_t recvSize;
    unsigned int priority;
    boost::posix_time::ptime t(boost::posix_time::second_clock::local_time() + boost::posix_time::time_duration(0,0,time));
    received = m_receiveMessageQueue->timed_receive(&msg, sizeof(SyncShmMessage), recvSize, priority, t);
    return msg;
}

SyncShmMessage::SyncShmMessage(int objectID, int arrayID)
    : m_objectID(objectID)
    , m_array_ID(arrayID) {
}

int SyncShmMessage::objectID() const {
    return m_objectID;
}

int SyncShmMessage::arrayID() const {
    return m_array_ID;
}




