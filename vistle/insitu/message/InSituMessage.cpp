#include "InSituMessage.h"

#include <iostream>

#include <boost/interprocess/creation_tags.hpp>



using namespace insitu;
using namespace insitu::message;
using namespace vistle::message;


//bool insitu::message::InSituTcpMessage::m_initialized = false;
//boost::mpi::communicator insitu::message::InSituTcpMessage::m_comm;
//std::shared_ptr< boost::asio::ip::tcp::socket> insitu::message::InSituTcpMessage::m_socket;
//#ifndef MODULE_THREAD
//insitu::message::SyncShmIDs::ShmSegment insitu::message::SyncShmIDs::m_segment;
//#else
//insitu::message::SyncShmIDs::Data insitu::message::SyncShmIDs::m_segment;
//#endif // !MODULE_THREAD
//
//
//bool insitu::message::SyncShmIDs::m_initialized = false;
//int  insitu::message::SyncShmIDs::m_rank = -1;
//int  insitu::message::SyncShmIDs::m_moduleID = -1;



InSituMessageType InSituMessageBase::type() const {
    return m_type;
}

InSituMessageType insitu::message::Message::type() const {
    return m_type;
}

insitu::message::Message insitu::message::Message::errorMessage()
{
    return Message{};
}

insitu::message::Message::Message(InSituMessageType type, vistle::buffer&& payload)
    :m_type(type)
    , m_payload(payload) {
}


insitu::message::Message::Message()
    : m_type(InSituMessageType::Invalid) {

}







