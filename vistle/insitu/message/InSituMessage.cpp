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

insitu::message::SyncShmIDs::ShmSegment insitu::message::SyncShmIDs::m_segment;
bool insitu::message::SyncShmIDs::m_initialized = false;
int  insitu::message::SyncShmIDs::m_rank = -1;
int  insitu::message::SyncShmIDs::m_moduleID = -1;

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


void SyncShmIDs::initialize(int moduleID, int rank, int instance, Mode mode) {

    m_moduleID = moduleID;
    m_rank = rank;
    std::string segName = vistle::message::MessageQueue::createName(("syncShmIds" + std::to_string(instance)).c_str(), moduleID, rank);
    m_segment = ShmSegment(segName, mode);

    m_initialized = true;
}

void SyncShmIDs::close() {
    m_segment = ShmSegment{};
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
        *m_segment->find<int>("objID").first = objID;
        *m_segment->find<int>("arrayID").first = arrayID;
    } catch (const boost::interprocess::interprocess_exception& ex) {
        throw vistle::exception(std::string("error setting shm object IDs: ") + ex.what());
    }
}

int insitu::message::SyncShmIDs::objectID() {
    if (m_initialized) {
        try {
            return *m_segment->find<int>("objID").first;
        } catch (const boost::interprocess::interprocess_exception& ex) {
            throw vistle::exception(std::string("error getting shm object ID: ") + ex.what());
        }
    }
    return vistle::Shm::the().objectID();

}

int insitu::message::SyncShmIDs::arrayID() {
    if (m_initialized) {
    }try {
        return    *m_segment->find<int>("arrayID").first;

    } catch (const boost::interprocess::interprocess_exception& ex) {
        throw vistle::exception(std::string("error getting shm array ID: ") + ex.what());
    }
    return vistle::Shm::the().arrayID();

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

            m_segment.reset(new managed_shared_memory(create_only, name.c_str(), mapped_region::get_page_size()));
            int* objID = m_segment->construct<int>("objID")(vistle::Shm::the().objectID());
            int* arrayID = m_segment->construct<int>("arrayID")(vistle::Shm::the().arrayID());
        } catch (boost::interprocess::interprocess_exception & ex) {
            throw vistle::exception(std::string("opening shm segment ") + name + ": " + ex.what());
        }
    }

    break;
    case SyncShmIDs::Mode::Attach:
    {
        try {
            m_segment.reset(new managed_shared_memory(open_only, name.c_str()));
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

const std::unique_ptr<boost::interprocess::managed_shared_memory>& insitu::message::SyncShmIDs::ShmSegment::operator->() const{
    return m_segment;
}

std::unique_ptr<boost::interprocess::managed_shared_memory>& insitu::message::SyncShmIDs::ShmSegment::operator->() {
    return m_segment;
}

insitu::message::SyncShmIDs::ShmSegment& insitu::message::SyncShmIDs::ShmSegment::operator=(ShmSegment&& other) noexcept{
    m_segment = std::move(other.m_segment);
    m_name = std::move(other.m_name);
    return *this;
}




