#include "InSituMessage.h"
#include <boost/asio.hpp>
#include <iostream>

#include <boost/interprocess/creation_tags.hpp>

#include <core/messagequeue.h>

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

InSituMessageType insitu::message::InSituMessageBase::type() const {
    return m_type;
}

InSituMessageType InSituTcp::Message::type() const{
    return m_type;
}

InSituTcp::Message::Message(InSituMessageType type, vistle::buffer&& payload)
    :m_type(type)
    ,m_payload(payload){
}


InSituTcp::Message::Message()
    :m_type(InSituMessageType::Invalid) {

}

InSituTcp::Message InSituTcp::recv() {
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
        return InSituTcp::Message{};
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
    return InSituTcp::Message{static_cast<InSituMessageType>(type), std::move(payload)};
}

void InSituTcp::initialize(std::shared_ptr<boost::asio::ip::tcp::socket> socket, boost::mpi::communicator comm) {
    m_socket = socket;
    m_comm = comm;
    m_initialized = true;
}

bool InSituTcp::isInitialized() {
    return m_initialized;
}


void SyncShmIDs::initialize(int moduleID, int rank, int instance, Mode mode) {

    m_moduleID = moduleID;
    m_rank = rank;
    std::string segName = vistle::message::MessageQueue::createName(("syncShmIds" + std::to_string(instance)).c_str(), moduleID, rank);
#ifndef MODULE_THREAD
    m_segment = ShmSegment{ segName, mode };
#endif // MODULE_THREAD

    m_initialized = true;
}

void SyncShmIDs::close() {
#ifndef MODULE_THREAD
    m_segment = ShmSegment{};
#endif // MODULE_THREAD


    m_initialized = false;

}

bool SyncShmIDs::isInitialized() {
    return m_initialized;
}

void insitu::message::SyncShmIDs::set(int objID, int arrayID) {
    if (!m_initialized) {
        return;
    }
    try {
        Guard g(m_segment.data()->mutex);
        m_segment.data()->objID = objID;
        m_segment.data()->arrayID = arrayID;
    } catch (const boost::interprocess::interprocess_exception& ex) {
        throw vistle::exception(std::string("error setting shm object IDs: ") + ex.what());
    }
}

int insitu::message::SyncShmIDs::objectID() {
    if (m_initialized) {
        try {
            Guard g(m_segment.data()->mutex);
            return m_segment.data()->objID;
        } catch (const boost::interprocess::interprocess_exception& ex) {
            throw vistle::exception(std::string("error getting shm object ID: ") + ex.what());
        }
    }
    return vistle::Shm::the().objectID();

}

int insitu::message::SyncShmIDs::arrayID() {
    if (m_initialized) {
    }try {
        Guard g(m_segment.data()->mutex);
        return m_segment.data()->arrayID;

    } catch (const boost::interprocess::interprocess_exception& ex) {
        throw vistle::exception(std::string("error getting shm array ID: ") + ex.what());
    }
    return vistle::Shm::the().arrayID();

}

#ifndef MODULE_THREAD

insitu::message::SyncShmIDs::ShmSegment::ShmSegment(insitu::message::SyncShmIDs::ShmSegment&& other) 
:m_name(std::move(other.m_name))
,m_region(std::move(other.m_region)) 

{

}

insitu::message::SyncShmIDs::ShmSegment::ShmSegment(const std::string& name, Mode mode) throw(vistle::exception)
:m_name(name){
    using namespace boost::interprocess;
    std::cerr << "trying to create shm segment with name " << name << std::endl;
    switch (mode) {
    case SyncShmIDs::Mode::Create:
    {
        try {

            shared_memory_object::remove(name.c_str());

            auto m_shmObj = shared_memory_object { create_only, name.c_str(), read_write };
            m_shmObj.truncate(sizeof(ShmData));
            m_region = mapped_region{ m_shmObj, read_write };
            void* addr = m_region.get_address();
            auto data = new(addr)ShmData{vistle::Shm::the().objectID(), vistle::Shm::the().arrayID()};
        } catch (boost::interprocess::interprocess_exception & ex) {
            throw vistle::exception(std::string("opening shm segment ") + name + ": " + ex.what());
        }
    }

    break;
    case SyncShmIDs::Mode::Attach:
    {
        try {
            auto m_shmObj = shared_memory_object{ open_only, name.c_str(), read_write };
            m_region = mapped_region{ m_shmObj, read_write };
            shared_memory_object::remove(name.c_str());
        } catch (boost::interprocess::interprocess_exception & ex) {
            throw vistle::exception(std::string("opening shm segment ") + name + ": " + ex.what());
        }
    }
    break;
    default:
        break;
    }


}



insitu::message::SyncShmIDs::ShmSegment::~ShmSegment() {
    boost::interprocess::shared_memory_object::remove(m_name.c_str());
}

const insitu::message::SyncShmIDs::ShmData* insitu::message::SyncShmIDs::ShmSegment::data() const {
    return static_cast<ShmData*>(m_region.get_address());
}
insitu::message::SyncShmIDs::ShmData* insitu::message::SyncShmIDs::ShmSegment::data()  {
    return static_cast<ShmData *>(m_region.get_address());
}

insitu::message::SyncShmIDs::ShmSegment& insitu::message::SyncShmIDs::ShmSegment::operator=(insitu::message::SyncShmIDs::ShmSegment&& other) noexcept{
    m_name = std::move(other.m_name);
    m_region = std::move(other.m_region);
    return *this;
}

#endif // !MODULE_THREAD




